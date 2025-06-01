#ifndef SD_CARD_MANAGER_H
#define SD_CARD_MANAGER_H

#include <Arduino.h>
#include "FS.h"   // Comes with ESP32 core
#include "SD.h"   // Comes with ESP32 core
#include "SPI.h"  // Comes with ESP32 core
#include "config.h" // For SD_CS_PIN

// Call this in main setup()
// Returns true if SD card is successfully initialized, false otherwise.
bool setupSdCard();

// Check if SD card is mounted and available
bool isSdCardAvailable();

// Lists directory contents to Serial.
void listDirectory(const char *dirname, uint8_t levels);

// Creates a directory.
// Returns true on success, false on failure.
bool createSdDir(const char *path);

// Removes a directory.
// Returns true on success, false on failure. (Directory must be empty)
bool removeSdDir(const char *path);

// Reads the entire content of a file into a String.
// Returns the file content, or an empty String on failure.
String readSdFile(const char *path);

// Writes a String to a file, overwriting existing content.
// Returns true on success, false on failure.
bool writeSdFile(const char *path, const char *message);

// Appends a String to a file.
// Returns true on success, false on failure.
bool appendSdFile(const char *path, const char *message);

// Renames a file or directory.
// Returns true on success, false on failure.
bool renameSdFile(const char *path1, const char *path2);

// Deletes a file.
// Returns true on success, false on failure.
bool deleteSdFile(const char *path);

// Gets the total size of the SD card in MegaBytes.
// Returns 0 if SD card is not available.
uint64_t getSdTotalSpaceMB();

// Gets the used space on the SD card in MegaBytes.
// Returns 0 if SD card is not available.
uint64_t getSdUsedSpaceMB();

// Perform a basic I/O test (for debugging purposes)
void testSdFileIO(const char *path);

// Directory structure constants (can be used by other modules)
#define WIFI_DIR        "/wifi"
#define KNOWN_WIFI_FILE_PATH (WIFI_DIR "/known.txt") // <--- ADDED
#define MUSIC_DIR       "/music"
#define CAPTURES_DIR    "/captures"
#define HANDSHAKES_DIR  (CAPTURES_DIR "/handshakes")
#define LOGS_DIR        "/logs"
#define SETTINGS_DIR    "/settings"

#endif // SD_CARD_MANAGER_H