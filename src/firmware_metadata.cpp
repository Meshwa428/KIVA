#include "firmware_metadata.h"
#include <ArduinoJson.h> // For parsing/generating metadata files
#include <MD5Builder.h>  // For calculating MD5 checksums
#include <SD.h> // <--- ADD THIS INCLUDE

// Global definitions for the extern variables in firmware_metadata.h
FirmwareInfo availableSdFirmwares[MAX_FIRMWARES_ON_SD];
int availableSdFirmwareCount = 0;


bool parseFirmwareMetadataFile(const String& kfwFilePath, FirmwareInfo& info) {
    if (!SD.exists(kfwFilePath)) {
        Serial.printf("Metadata: File not found: %s\n", kfwFilePath.c_str());
        info.isValid = false;
        return false;
    }

    File metaFile = SD.open(kfwFilePath, FILE_READ);
    if (!metaFile) {
        Serial.printf("Metadata: Failed to open %s\n", kfwFilePath.c_str());
        info.isValid = false;
        return false;
    }

    // Adjust JSON document size as needed based on metadata complexity
    StaticJsonDocument<512> doc; // Increased size
    DeserializationError error = deserializeJson(doc, metaFile);
    metaFile.close();

    if (error) {
        Serial.printf("Metadata: deserializeJson() failed for %s: %s\n", kfwFilePath.c_str(), error.c_str());
        info.isValid = false;
        return false;
    }

    strlcpy(info.version, doc["version"] | "", FW_VERSION_MAX_LEN);
    strlcpy(info.build_date, doc["build_date"] | "", FW_BUILD_DATE_MAX_LEN);
    strlcpy(info.checksum_md5, doc["checksum_md5"] | "", FW_CHECKSUM_MD5_MAX_LEN);
    strlcpy(info.binary_filename, doc["binary_filename"] | "", FW_BINARY_FILENAME_MAX_LEN);
    strlcpy(info.description, doc["description"] | "", FW_DESCRIPTION_MAX_LEN);
    
    // Basic validation: checksum and binary_filename must exist
    if (strlen(info.checksum_md5) != 32 || strlen(info.binary_filename) == 0) {
        Serial.printf("Metadata: Invalid or missing checksum/binary_filename in %s\n", kfwFilePath.c_str());
        info.isValid = false;
        return false;
    }

    info.isValid = true;
    return true;
}

bool saveFirmwareMetadataFile(const String& kfwFilePath, const FirmwareInfo& info) {
    File metaFile = SD.open(kfwFilePath, FILE_WRITE);
    if (!metaFile) {
        Serial.printf("Metadata: Failed to open %s for writing\n", kfwFilePath.c_str());
        return false;
    }

    StaticJsonDocument<512> doc;
    doc["version"] = info.version;
    doc["build_date"] = info.build_date;
    doc["checksum_md5"] = info.checksum_md5;
    doc["binary_filename"] = info.binary_filename;
    doc["description"] = info.description;

    if (serializeJson(doc, metaFile) == 0) {
        Serial.printf("Metadata: serializeJson() failed for %s\n", kfwFilePath.c_str());
        metaFile.close();
        return false;
    }

    metaFile.close();
    Serial.printf("Metadata: Saved %s\n", kfwFilePath.c_str());
    return true;
}

String calculateFileMD5(fs::FS &fs, const String& filePath) {
    File file = fs.open(filePath, FILE_READ);
    if (!file || file.isDirectory()) {
        Serial.printf("MD5: Failed to open file or it's a directory: %s\n", filePath.c_str());
        return "";
    }

    MD5Builder md5;
    md5.begin();
    
    // Stream the file to MD5 object
    // size_t fileSize = file.size(); // Not strictly needed for md5.update
    uint8_t buffer[512];
    size_t bytesRead;
    while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
        md5.add(buffer, bytesRead);
    }
    file.close();

    md5.calculate();
    String md5Result = md5.toString();
    md5Result.toLowerCase(); // Ensure consistent case
    return md5Result;
}
