#include "SdCardManager.h"
#include "Config.h"

namespace SdCardManager {
    static bool sdCardInitialized = false;

    // --- NEW: LineReader Implementation ---
    LineReader::LineReader() : file_() {}

    LineReader::LineReader(File file) : file_(file) {}

    LineReader::~LineReader() {
        if (file_) {
            file_.close();
        }
    }

    // Move constructor
    LineReader::LineReader(LineReader&& other) noexcept : file_(other.file_) {
        other.file_ = File(); // Invalidate other's file handle
    }

    // Move assignment
    LineReader& LineReader::operator=(LineReader&& other) noexcept {
        if (this != &other) {
            if (file_) {
                file_.close();
            }
            file_ = other.file_;
            other.file_ = File();
        }
        return *this;
    }

    bool LineReader::isOpen() const {
        return (bool)file_;
    }

    void LineReader::close() {
        if (file_) {
            file_.close();
        }
    }

    String LineReader::readLine() {
        if (!file_ || file_.size() == 0) {
            return "";
        }
    
        while (true) {
            uint32_t search_start_pos = file_.position();
            
            while (file_.available()) {
                String line = file_.readStringUntil('\n');
                line.trim();
                if (!line.isEmpty()) {
                    return line;
                }
            }
    
            file_.seek(0);
    
            if (search_start_pos == 0) {
                return "";
            }
        }
    }

    LineReader openLineReader(const char* path) {
        if (!sdCardInitialized) {
            return LineReader();
        }
        File f = SD.open(path, FILE_READ);
        if (!f || f.isDirectory()) {
            if (f) f.close();
            return LineReader();
        }
        return LineReader(f);
    }
    
    // --- Existing function implementations ---
    bool setup() {
        if (!SD.begin(Pins::SD_CS_PIN)) {
            Serial.println("[SD-LOG] Mount Failed");
            sdCardInitialized = false;
            return false;
        }
        sdCardInitialized = true;
        Serial.println("[SD-LOG] SD Card Mounted.");
        ensureStandardDirs();
        return true;
    }

    bool isAvailable() {
        return sdCardInitialized;
    }

    bool exists(const char* path) {
        if (!sdCardInitialized) return false;
        return SD.exists(path);
    }

    void ensureStandardDirs() {
        if (!isAvailable()) return;

        const char* dirsToCreate[] = {
            SD_ROOT::CONFIG,
            SD_ROOT::DATA,
            SD_ROOT::USER,
            SD_ROOT::FIRMWARE,
            SD_ROOT::WEB,
            SD_ROOT::DATA_LOGS,
            SD_ROOT::DATA_CAPTURES,
            SD_ROOT::DATA_PROBES,
            SD_ROOT::USER_BEACON_LISTS,
            SD_ROOT::USER_PORTALS,
            SD_ROOT::USER_DUCKY
        };

        for (const char* dir : dirsToCreate) {
            if (!exists(dir)) {
                createDir(dir);
            }
        }
    }

    bool createDir(const char *path) {
        if (!sdCardInitialized) return false;
        if (SD.mkdir(path)) {
            Serial.printf("[SD-LOG] Created directory: %s\n", path);
            return true;
        }
        Serial.printf("[SD-LOG] Failed to create directory: %s\n", path);
        return false;
    }

    String readFile(const char *path) {
        if (!sdCardInitialized) return "";
        File file = SD.open(path, FILE_READ);
        if (!file || file.isDirectory()) {
            if (file) file.close();
            return "";
        }
        String content = "";
        while (file.available()) {
            content += (char)file.read();
        }
        file.close();
        return content;
    }

    bool writeFile(const char *path, const char *message) {
        if (!sdCardInitialized) return false;
        File file = SD.open(path, FILE_WRITE);
        if (!file) {
            Serial.printf("[SD-LOG] Failed to open file for writing: %s\n", path);
            return false;
        }
        if (file.print(message)) {
            file.close();
            return true;
        }
        file.close();
        return false;
    }

    bool deleteFile(const char* path) {
        if (!sdCardInitialized) return false;
        return SD.remove(path);
    }
    
    bool renameFile(const char* pathFrom, const char* pathTo) {
        if (!sdCardInitialized) return false;
        return SD.rename(pathFrom, pathTo);
    }

    File openFile(const char* path, const char* mode) {
        if (!sdCardInitialized) {
            return File();
        }
        return SD.open(path, mode);
    }

}