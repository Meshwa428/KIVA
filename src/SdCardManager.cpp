#include "SdCardManager.h"
#include "Config.h" // For Pins::SD_CS_PIN

namespace SdCardManager {
    static bool sdCardInitialized = false;

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
            // Serial.printf("[SD-LOG] Failed to open file for reading: %s\n", path);
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
}