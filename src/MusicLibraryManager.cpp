#include "MusicLibraryManager.h"
#include "App.h"
#include "Logger.h"

MusicLibraryManager::MusicLibraryManager() : app_(nullptr) {}

void MusicLibraryManager::setup(App* app) {
    app_ = app;
}

void MusicLibraryManager::buildIndex() {
    LOG(LogLevel::INFO, "MUSIC_LIB", "Starting distributed music library indexing...");
    createIndexForDirectory(SD_ROOT::USER_MUSIC);
    LOG(LogLevel::INFO, "MUSIC_LIB", "Music library indexing complete.");
}

void MusicLibraryManager::createIndexForDirectory(const char* path) {
    // 1. Create the index file for the current directory
    std::string indexPath = std::string(path) + "/" + INDEX_FILENAME;
    
    // Always create a fresh index
    if (SdCardManager::exists(indexPath.c_str())) {
        SdCardManager::deleteFile(indexPath.c_str());
    }

    File indexFile = SdCardManager::openFile(indexPath.c_str(), FILE_WRITE);
    if (!indexFile) {
        LOG(LogLevel::ERROR, "MUSIC_LIB", "Failed to create index file at %s", path);
        return;
    }

    // 2. Scan the directory for items
    File root = SdCardManager::openFile(path);
    if (!root || !root.isDirectory()) {
        if (root) root.close();
        indexFile.close();
        return;
    }

    File file = root.openNextFile();
    while (file) {
        std::string fileName = file.name();
        // Ignore dotfiles, the index file itself, and other system files
        if (fileName == "." || fileName == ".." || fileName == INDEX_FILENAME) {
            file.close(); file = root.openNextFile(); continue;
        }

        if (file.isDirectory()) {
            // It's a playlist. Write its info to the current index file.
            // Format: P;DisplayName;FullPath
            indexFile.printf("P;%s;%s\n", fileName.c_str(), file.path());
            
            // --- RECURSION ---
            // Now, go create an index inside that subdirectory.
            createIndexForDirectory(file.path());

        } else if (String(file.name()).endsWith(".mp3")) {
            // It's a track. Write its info.
            // Format: T;DisplayName;FullPath
            indexFile.printf("T;%s;%s\n", fileName.c_str(), file.path());
        }
        
        file.close();
        file = root.openNextFile();
    }
    
    root.close();
    indexFile.close();
}