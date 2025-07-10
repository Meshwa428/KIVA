#ifndef SD_CARD_MANAGER_H
#define SD_CARD_MANAGER_H

#include <Arduino.h>
#include <SD.h>

// Standard directory paths
#define WIFI_DIR        "/wifi"
#define KNOWN_WIFI_FILE (WIFI_DIR "/known.txt")
#define OTA_HTML_PAGE_PATH "/system/ota_page.html" // Define here for OtaManager

namespace SdCardManager {
    // Call in main App::setup()
    bool setup();
    bool isAvailable();
    
    bool exists(const char* path);

    // High-level file operations
    bool createDir(const char* path);
    String readFile(const char* path);
    bool writeFile(const char* path, const char* message);
    bool deleteFile(const char *path);
    bool renameFile(const char* pathFrom, const char* pathTo);

    // Low-level access for streaming operations (like OTA)
    // This is the controlled exception to the abstraction.
    File openFile(const char* path, const char* mode = FILE_READ);

    void ensureStandardDirs();
};

#endif // SD_CARD_MANAGER_H