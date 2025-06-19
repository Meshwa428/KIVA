#ifndef FIRMWARE_METADATA_H
#define FIRMWARE_METADATA_H

#include <Arduino.h>
#include <FS.h> // For File
#include "config.h" // For MAX_FIRMWARES_ON_SD

#define FW_VERSION_MAX_LEN 24
#define FW_BUILD_DATE_MAX_LEN 24
#define FW_CHECKSUM_MD5_MAX_LEN 33 // 32 chars + null
#define FW_BINARY_FILENAME_MAX_LEN 48 // Max length for binary file name (e.g., firmware_XXXXXXXX.bin)
#define FW_DESCRIPTION_MAX_LEN 64

struct FirmwareInfo {
    char version[FW_VERSION_MAX_LEN];
    char build_date[FW_BUILD_DATE_MAX_LEN]; // ISO 8601 format ideally
    char checksum_md5[FW_CHECKSUM_MD5_MAX_LEN];
    char binary_filename[FW_BINARY_FILENAME_MAX_LEN]; // e.g., "my_firmware_v1.bin"
    char description[FW_DESCRIPTION_MAX_LEN];
    bool isValid; // Flag to indicate if the struct holds valid data

    FirmwareInfo() : isValid(false) {
        version[0] = '\0';
        build_date[0] = '\0';
        checksum_md5[0] = '\0';
        binary_filename[0] = '\0';
        description[0] = '\0';
    }
};

// Functions for handling firmware metadata
bool parseFirmwareMetadataFile(const String& kfwFilePath, FirmwareInfo& info);
bool saveFirmwareMetadataFile(const String& kfwFilePath, const FirmwareInfo& info);
String calculateFileMD5(fs::FS &fs, const String& filePath);

// To store the list of firmwares found on SD card for the FIRMWARE_SD_LIST_MENU
extern FirmwareInfo availableSdFirmwares[MAX_FIRMWARES_ON_SD];
extern int availableSdFirmwareCount;

#endif // FIRMWARE_METADATA_H