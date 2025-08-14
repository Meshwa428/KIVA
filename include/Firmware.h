#ifndef FIRMWARE_H
#define FIRMWARE_H

#include <Arduino.h>
#include <FS.h>

#define FW_VERSION_MAX_LEN 24
#define FW_BUILD_DATE_MAX_LEN 24
#define FW_CHECKSUM_MD5_MAX_LEN 33 // 32 chars + null
#define FW_BINARY_FILENAME_MAX_LEN 48
#define FW_DESCRIPTION_MAX_LEN 64

struct FirmwareInfo {
    char version[FW_VERSION_MAX_LEN];
    char build_date[FW_BUILD_DATE_MAX_LEN];
    char checksum_md5[FW_CHECKSUM_MD5_MAX_LEN];
    char binary_filename[FW_BINARY_FILENAME_MAX_LEN];
    char description[FW_DESCRIPTION_MAX_LEN];
    bool isValid;

    FirmwareInfo() : isValid(false) {
        version[0] = '\0';
        build_date[0] = '\0';
        checksum_md5[0] = '\0';
        binary_filename[0] = '\0';
        description[0] = '\0';
    }
};

namespace FirmwareUtils {
    bool parseMetadataFile(const String& kfwFilePath, FirmwareInfo& info);
    bool saveMetadataFile(const String& kfwFilePath, const FirmwareInfo& info);
    String calculateFileMD5(fs::FS &fs, const String& filePath);
}

#endif // FIRMWARE_H