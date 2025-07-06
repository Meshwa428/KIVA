#ifndef SD_CARD_MANAGER_H
#define SD_CARD_MANAGER_H

#include <Arduino.h>
#include <SD.h>

// Standard directory paths
#define WIFI_DIR        "/wifi"
#define KNOWN_WIFI_FILE (WIFI_DIR "/known.txt")

namespace SdCardManager {
    // Call in main App::setup()
    bool setup();
    bool isAvailable();
    
    bool exists(const char* path);

    bool createDir(const char* path);
    String readFile(const char* path);
    bool writeFile(const char* path, const char* message);
    bool deleteFile(const char *path);

    void ensureStandardDirs();
};

#endif // SD_CARD_MANAGER_H