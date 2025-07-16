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
    
        // This loop handles the "forever" part, but with a safety exit.
        while (true) {
            // Remember where we started the search in the file.
            uint32_t search_start_pos = file_.position();
            
            // Search for a non-empty line from the current position to the end.
            while (file_.available()) {
                String line = file_.readStringUntil('\n');
                line.trim();
                if (!line.isEmpty()) {
                    return line; // Found a valid line, return it.
                }
            }
    
            // If we get here, we've hit the end of the file. Rewind to the beginning.
            file_.seek(0);
    
            // SAFETY BREAK: If our search started at the very beginning (pos 0)
            // and we read the entire file without finding a single valid line,
            // it means the file is composed entirely of whitespace. We must
            // return an empty string to signal this and prevent an infinite loop.
            if (search_start_pos == 0) {
                return "";
            }
            
            // If we started mid-file, we now continue the loop, searching from the top.
        }
    }

    // --- NEW: Factory Function Implementation ---
    LineReader openLineReader(const char* path) {
        if (!sdCardInitialized) {
            return LineReader(); // Return a closed/invalid reader
        }
        File f = SD.open(path, FILE_READ);
        if (!f || f.isDirectory()) {
            if (f) f.close();
            return LineReader(); // Return a closed/invalid reader
        }
        // Construct a LineReader from the valid File object
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
        const char *dirs[] = {"/wifi", "/firmware", "/system", "/captures", "/logs"};
        for (const char* dir : dirs) {
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

    // Implementation of the new function
    File openFile(const char* path, const char* mode) {
        if (!sdCardInitialized) {
            return File(); // Return an invalid/closed File object
        }
        return SD.open(path, mode);
    }

}