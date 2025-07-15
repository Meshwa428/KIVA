#ifndef SD_CARD_MANAGER_H
#define SD_CARD_MANAGER_H

#include <Arduino.h>
#include <SD.h>

// Standard directory paths
#define WIFI_DIR        "/wifi"
#define KNOWN_WIFI_FILE (WIFI_DIR "/known.txt")
#define OTA_HTML_PAGE_PATH "/system/ota_page.html"

namespace SdCardManager {
    // --- NEW: LineReader Class ---
    /**
     * @brief An abstract line-by-line file reader.
     * Encapsulates a File object to remove direct dependency on the SD library
     * for components that need to stream file content.
     */
    class LineReader {
    public:
        LineReader();
        ~LineReader();
        LineReader(LineReader&& other) noexcept; // Move constructor
        LineReader& operator=(LineReader&& other) noexcept; // Move assignment

        String readLine();
        bool isOpen() const;
        void close();

        // Disable copy constructor and assignment
        LineReader(const LineReader&) = delete;
        LineReader& operator=(const LineReader&) = delete;

    private:
        friend LineReader openLineReader(const char* path); // Allow factory function to access private constructor
        LineReader(File file); // Private constructor taking a File object
        File file_;
    };

    // --- NEW: Factory function to create a LineReader ---
    LineReader openLineReader(const char* path);


    // --- Existing Functions ---
    bool setup();
    bool isAvailable();
    bool exists(const char* path);
    bool createDir(const char* path);
    String readFile(const char* path);
    bool writeFile(const char* path, const char* message);
    bool deleteFile(const char *path);
    bool renameFile(const char* pathFrom, const char* pathTo);
    File openFile(const char* path, const char* mode = FILE_READ); // Still useful for OTA binary streaming
    void ensureStandardDirs();
};

#endif // SD_CARD_MANAGER_H