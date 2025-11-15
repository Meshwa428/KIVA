#include "MusicLibraryManager.h"
#include "App.h"
#include "Logger.h"
#include <Arduino.h> // For pgm_read_word/dword
#include <vector>
#include <string>
#include <algorithm>

// --- Data structure for in-memory index entries ---
struct IndexEntry {
    char type; // 'P' for playlist (directory), 'T' for track (file)
    std::string name;
    std::string path;
    int duration; // Only for tracks

    // For sorting and searching by path
    bool operator<(const IndexEntry& other) const {
        return path < other.path;
    }
};

// Bitrates in kbps. Index: [version_idx][layer_idx][bitrate_idx]
// version_idx: 0=MPEG2/2.5, 1=MPEG1
// layer_idx: 0=reserved, 1=Layer III, 2=Layer II, 3=Layer I
static const uint16_t PROGMEM BITRATE_MAP[2][4][16] = {
    { // MPEG 2 & 2.5
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // Reserved
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}, // Layer III
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}, // Layer II
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0}  // Layer I
    },
    { // MPEG 1
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // Reserved
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}, // Layer III
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0},    // Layer II
        {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0}   // Layer I
    }
};
// Sample rates in Hz. Index: [version_idx][samplerate_idx]
// version_idx: 0=MPEG1, 1=MPEG2, 2=MPEG2.5
static const uint32_t PROGMEM SAMPLERATE_MAP[3][4] = {
    {44100, 48000, 32000, 0}, // MPEG 1
    {22050, 24000, 16000, 0}, // MPEG 2
    {11025, 12000, 8000, 0}   // MPEG 2.5
};
// Samples per frame. Index: [version_idx][layer_idx]
static const uint16_t PROGMEM SAMPLES_PER_FRAME_MAP[2][4] = {
    // MPEG 2 & 2.5 (Layer 1=III, 2=II, 3=I)
    {0, 576, 1152, 384},
    // MPEG 1 (Layer 1=III, 2=II, 3=I)
    {0, 1152, 1152, 384}
};
int getMp3Duration(const char* path) {
    File file = SdCardManager::getInstance().openFileUncached(path, FILE_READ);
    if (!file) return 0;

    uint8_t header[10];
    if (file.read(header, 10) != 10) { file.close(); return 0; }

    uint32_t audio_data_start = 0;
    if (header[0] == 'I' && header[1] == 'D' && header[2] == '3') {
        audio_data_start = ((header[6] & 0x7F) << 21) | ((header[7] & 0x7F) << 14) | ((header[8] & 0x7F) << 7) | (header[9] & 0x7F);
        audio_data_start += 10;
    }
    
    file.seek(audio_data_start);
    uint32_t first_frame_header = 0;
    uint32_t first_frame_pos = 0;
    int b1 = 0, b2 = 0;
    while(true) {
        b1 = b2; b2 = file.read();
        if (b2 < 0) { file.close(); return 0; }
        if (b1 == 0xFF && (b2 & 0xE0) == 0xE0) {
            first_frame_header = (b1 << 24) | (b2 << 16) | (file.read() << 8) | file.read();
            first_frame_pos = file.position() - 4;
            break;
        }
    }

    if ((first_frame_header & 0xFFE00000) != 0xFFE00000) { file.close(); return 0; }
    int mpeg_version_bits = (first_frame_header >> 19) & 3;
    int layer_bits = (first_frame_header >> 17) & 3;
    int bitrate_idx = (first_frame_header >> 12) & 15;
    int samplerate_idx = (first_frame_header >> 10) & 3;
    int channel_mode = (first_frame_header >> 6) & 3;
    bool is_mono = (channel_mode == 3);
    
    if (mpeg_version_bits == 1 || layer_bits == 0 || bitrate_idx == 0 || bitrate_idx == 15 || samplerate_idx == 3) { file.close(); return 0; }

    int version_idx_sr;
    if (mpeg_version_bits == 3) version_idx_sr = 0;
    else if (mpeg_version_bits == 2) version_idx_sr = 1;
    else version_idx_sr = 2;

    int version_idx_br_spf = (mpeg_version_bits == 3) ? 1 : 0;
    int layer_idx = layer_bits;

    uint32_t samplerate = pgm_read_dword(&SAMPLERATE_MAP[version_idx_sr][samplerate_idx]);
    uint16_t bitrate = pgm_read_word(&BITRATE_MAP[version_idx_br_spf][layer_idx][bitrate_idx]) * 1000;
    uint16_t samples_per_frame = pgm_read_word(&SAMPLES_PER_FRAME_MAP[version_idx_br_spf][layer_idx]);

    if (samplerate == 0 || samples_per_frame == 0) { file.close(); return 0; }

    uint32_t total_frames = 0;
    int xing_offset = (version_idx_br_spf == 1) ? (is_mono ? 21 : 36) : (is_mono ? 13 : 21);
    
    file.seek(first_frame_pos + xing_offset);
    uint8_t xing_header[12];
    if (file.read(xing_header, 12) == 12 && (memcmp(xing_header, "Xing", 4) == 0 || memcmp(xing_header, "Info", 4) == 0)) {
        if (xing_header[7] & 0x01) { total_frames = (xing_header[8] << 24) | (xing_header[9] << 16) | (xing_header[10] << 8) | xing_header[11]; }
    }
    
    if (total_frames == 0) {
        file.seek(first_frame_pos + 36);
        if (file.read(xing_header, 4) == 4 && memcmp(xing_header, "VBRI", 4) == 0) {
            file.seek(file.position() + 10);
            if (file.read(xing_header, 4) == 4) { total_frames = (xing_header[0] << 24) | (xing_header[1] << 16) | (xing_header[2] << 8) | xing_header[3]; }
        }
    }

    if (total_frames > 0) {
        file.close();
        return (uint64_t)total_frames * samples_per_frame / samplerate;
    }

    if (bitrate > 0) {
        uint32_t audio_data_size = file.size() - audio_data_start;
        file.close();
        return (uint64_t)audio_data_size * 8 / bitrate;
    }
    
    file.close();
    return 0;
}


MusicLibraryManager::MusicLibraryManager() : app_(nullptr) {}

void MusicLibraryManager::setup(App* app) {
    app_ = app;
}

// Private helper function prototype
void syncIndexForDirectory(const char* path);

void MusicLibraryManager::buildIndex() {
    LOG(LogLevel::INFO, "MUSIC_LIB", "Starting incremental music library sync...");
    syncIndexForDirectory(SD_ROOT::USER_MUSIC);
    LOG(LogLevel::INFO, "MUSIC_LIB", "Music library sync complete.");
}

// This new function replaces the old createIndexForDirectory
void syncIndexForDirectory(const char* path) {
    LOG(LogLevel::INFO, "MUSIC_SYNC", "Syncing directory: %s", path);
    std::string indexPath = std::string(path) + "/" + MusicLibraryManager::INDEX_FILENAME;
    
    // 1. Read existing index into memory
    std::vector<IndexEntry> oldEntries;
    auto oldIndexReader = SdCardManager::getInstance().openLineReader(indexPath.c_str());
    if (oldIndexReader.isOpen()) {
        while(true) {
            String line = oldIndexReader.readLine();
            if (line.isEmpty()) break;
            
            IndexEntry entry;
            entry.type = line.charAt(0);
            
            int firstSep = line.indexOf(';');
            int secondSep = line.indexOf(';', firstSep + 1);
            int thirdSep = (entry.type == 'T') ? line.indexOf(';', secondSep + 1) : -1;
            
            if (firstSep != -1 && secondSep != -1) {
                if (entry.type == 'T' && thirdSep != -1) {
                    entry.name = line.substring(firstSep + 1, secondSep).c_str();
                    entry.path = line.substring(secondSep + 1, thirdSep).c_str();
                    entry.duration = line.substring(thirdSep + 1).toInt();
                    oldEntries.push_back(entry);
                } else if (entry.type == 'P') {
                    entry.name = line.substring(firstSep + 1, secondSep).c_str();
                    entry.path = line.substring(secondSep + 1).c_str();
                    entry.duration = 0;
                    oldEntries.push_back(entry);
                }
            }
        }
        oldIndexReader.close();
    }

    // 2. Verify existing entries and create a quick lookup list of paths
    std::vector<IndexEntry> finalEntries;
    std::vector<std::string> verifiedPaths;
    for (const auto& entry : oldEntries) {
        if (SdCardManager::getInstance().exists(entry.path.c_str())) {
            finalEntries.push_back(entry);
            verifiedPaths.push_back(entry.path);
        } else {
            LOG(LogLevel::INFO, "MUSIC_SYNC", "Entry no longer exists, removing: %s", entry.path.c_str());
        }
    }
    std::sort(verifiedPaths.begin(), verifiedPaths.end());

    // 3. Scan the actual directory for new files and process them
    File root = SdCardManager::getInstance().openFileUncached(path);
    if (!root || !root.isDirectory()) {
        if (root) root.close();
        return; // Cannot proceed if directory is unreadable
    }

    File file = root.openNextFile();
    while (file) {
        std::string fileName = file.name();
        std::string filePath = file.path();

        if (fileName != "." && fileName != ".." && fileName != MusicLibraryManager::INDEX_FILENAME) {
            // Check if this path is already in our verified list using binary search for efficiency
            if (!std::binary_search(verifiedPaths.begin(), verifiedPaths.end(), filePath)) {
                LOG(LogLevel::INFO, "MUSIC_SYNC", "Found new item, processing: %s", filePath.c_str());
                if (file.isDirectory()) {
                    finalEntries.push_back({'P', fileName, filePath, 0});
                } else if (String(file.name()).endsWith(".mp3")) {
                    int duration = getMp3Duration(filePath.c_str());
                    finalEntries.push_back({'T', fileName, filePath, duration});
                }
            }
        }
        
        file.close();
        file = root.openNextFile();
    }
    root.close();

    // 4. Sort the final list for consistent output and write the new index file
    std::sort(finalEntries.begin(), finalEntries.end(), [](const IndexEntry& a, const IndexEntry& b){
        if (a.type != b.type) return a.type < b.type; // Playlists ('P') before Tracks ('T')
        return a.name < b.name;
    });

    File newIndexFile = SdCardManager::getInstance().openFileUncached(indexPath.c_str(), FILE_WRITE);
    if (!newIndexFile) {
        LOG(LogLevel::ERROR, "MUSIC_SYNC", "Failed to open new index file for writing: %s", indexPath.c_str());
        return;
    }

    for (const auto& entry : finalEntries) {
        if (entry.type == 'P') {
            newIndexFile.printf("P;%s;%s\n", entry.name.c_str(), entry.path.c_str());
        } else if (entry.type == 'T') {
            newIndexFile.printf("T;%s;%s;%d\n", entry.name.c_str(), entry.path.c_str(), entry.duration);
        }
    }
    newIndexFile.close();
    LOG(LogLevel::INFO, "MUSIC_SYNC", "Wrote %d entries to new index at %s", finalEntries.size(), indexPath.c_str());

    // 5. Recurse into all subdirectories (both old and new)
    for (const auto& entry : finalEntries) {
        if (entry.type == 'P') {
            syncIndexForDirectory(entry.path.c_str());
        }
    }
}