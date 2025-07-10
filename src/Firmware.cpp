#include "Firmware.h"
#include <ArduinoJson.h>
#include <MD5Builder.h>
#include <SD.h>

namespace FirmwareUtils {

bool parseMetadataFile(const String& kfwFilePath, FirmwareInfo& info) {
    if (!SD.exists(kfwFilePath)) {
        info.isValid = false;
        return false;
    }
    File metaFile = SD.open(kfwFilePath, FILE_READ);
    if (!metaFile) {
        info.isValid = false;
        return false;
    }
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, metaFile);
    metaFile.close();
    if (error) {
        info.isValid = false;
        return false;
    }
    strlcpy(info.version, doc["version"] | "", FW_VERSION_MAX_LEN);
    strlcpy(info.build_date, doc["build_date"] | "", FW_BUILD_DATE_MAX_LEN);
    strlcpy(info.checksum_md5, doc["checksum_md5"] | "", FW_CHECKSUM_MD5_MAX_LEN);
    strlcpy(info.binary_filename, doc["binary_filename"] | "", FW_BINARY_FILENAME_MAX_LEN);
    strlcpy(info.description, doc["description"] | "", FW_DESCRIPTION_MAX_LEN);
    if (strlen(info.checksum_md5) != 32 || strlen(info.binary_filename) == 0) {
        info.isValid = false;
        return false;
    }
    info.isValid = true;
    return true;
}

bool saveMetadataFile(const String& kfwFilePath, const FirmwareInfo& info) {
    File metaFile = SD.open(kfwFilePath, FILE_WRITE);
    if (!metaFile) return false;
    StaticJsonDocument<512> doc;
    doc["version"] = info.version;
    doc["build_date"] = info.build_date;
    doc["checksum_md5"] = info.checksum_md5;
    doc["binary_filename"] = info.binary_filename;
    doc["description"] = info.description;
    bool success = serializeJson(doc, metaFile) > 0;
    metaFile.close();
    return success;
}

String calculateFileMD5(fs::FS &fs, const String& filePath) {
    File file = fs.open(filePath, FILE_READ);
    if (!file || file.isDirectory()) {
        return "";
    }
    MD5Builder md5;
    md5.begin();
    md5.addStream(file, file.size());
    file.close();
    md5.calculate();
    return md5.toString();
}

}