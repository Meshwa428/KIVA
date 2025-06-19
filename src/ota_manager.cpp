#include "ota_manager.h"
#include "ui_drawing.h"
#include <FS.h>
#include <SPIFFS.h>
#include "sd_card_manager.h"
#include <esp_wifi.h>

const char favicon_ico_gz[] = {
  0x1f, 0x8b, 0x08, 0x08, 0x13, 0xb6, 0xa5, 0x62, 0x00, 0x03, 0x66, 0x61, 0x76, 0x69, 0x63, 0x6f, 0x6e, 0x2e, 0x69, 0x63, 0x6f, 0x00, 0xa5, 0x53, 0x69, 0x48,
  0x54, 0x51, 0x14, 0x7e, 0x41, 0x99, 0x4b, 0xa0, 0x15, 0x24, 0xb4, 0x99, 0x8e, 0x1b, 0x25, 0xa8, 0x54, 0xb4, 0x19, 0x56, 0x62, 0x41, 0xab, 0x14, 0x45, 0x61,
  0x51, 0x62, 0x86, 0x69, 0x1a, 0xd4, 0x60, 0xda, 0xa2, 0x99, 0x4b, 0x69, 0x1b, 0x42, 0x26, 0x5a, 0x09, 0x59, 0xd8, 0xfe, 0xab, 0x88, 0xa0, 0x82, 0xe8, 0x4f,
  0x90, 0xa5, 0x14, 0x68, 0x0b, 0xe6, 0x2c, 0x6f, 0x9c, 0xed, 0xcd, 0x9b, 0xcd, 0x19, 0x9d, 0x99, 0x3b, 0xe3, 0xd7, 0x79, 0x6f, 0x34, 0xac, 0xe8, 0x57, 0xf7,
  0x72, 0x1e, 0x87, 0xb3, 0xbe, 0xf3, 0x9d, 0xef, 0x72, 0xdc, 0x24, 0xba, 0x51, 0x51, 0x1c, 0x7d, 0x17, 0x70, 0x85, 0x93, 0x39, 0x6e, 0x16, 0xc7, 0x71, 0xc9,
  0x24, 0x64, 0x22, 0x4b, 0xd0, 0x2e, 0x1f, 0xf2, 0xcd, 0x8c, 0x08, 0xca, 0xf8, 0x89, 0x48, 0x54, 0x7b, 0x49, 0xd8, 0xb8, 0x84, 0xc6, 0xab, 0xd9, 0xd4, 0x58,
  0x15, 0x8b, 0x4e, 0xd1, 0xb0, 0xa4, 0x0c, 0x9e, 0x2d, 0x5a, 0xa3, 0x63, 0x31, 0x4b, 0xb4, 0x6c, 0x9a, 0x22, 0x68, 0x0f, 0x9f, 0x10, 0x3b, 0x21, 0x17, 0x92,
  0x84, 0xc5, 0xa9, 0x90, 0xbc, 0x8a, 0x47, 0xe1, 0x31, 0x01, 0x95, 0xf5, 0x22, 0x6a, 0x1a, 0xac, 0x28, 0x3f, 0x2b, 0xa2, 0x8a, 0x74, 0xe5, 0x19, 0x0b, 0xb2,
  0xb7, 0xeb, 0x11, 0x49, 0x71, 0xe1, 0x09, 0xc1, 0xf8, 0x09, 0x75, 0x10, 0xae, 0x50, 0x61, 0xf3, 0x6e, 0x03, 0x2a, 0xeb, 0x44, 0xe4, 0xec, 0x31, 0xa0, 0xf9,
  0x86, 0x1d, 0xb7, 0x3b, 0x9d, 0x58, 0x9e, 0xa5, 0xc3, 0xb5, 0x1b, 0x0e, 0xd4, 0x5f, 0xb4, 0xe2, 0xa8, 0x52, 0xc0, 0xf1, 0x53, 0x16, 0xcc, 0x49, 0xd3, 0x8c,
  0xd7, 0x90, 0xf3, 0xa5, 0xbe, 0xdb, 0x72, 0x0d, 0x78, 0xff, 0x71, 0x04, 0x77, 0x1f, 0x3a, 0x91, 0x9e, 0xc9, 0xe3, 0xed, 0xbb, 0x61, 0xb8, 0x87, 0x03, 0x28,
  0x56, 0x9a, 0xc1, 0x0f, 0x32, 0xf4, 0x7d, 0xf5, 0x42, 0x91, 0xaa, 0xc1, 0xba, 0x6d, 0x7a, 0xc4, 0xaf, 0xe0, 0x11, 0x16, 0x1f, 0xcc, 0x0f, 0xa5, 0xb9, 0x16,
  0xae, 0xe6, 0x51, 0x7b, 0xc9, 0x8a, 0x17, 0xaf, 0xdd, 0x18, 0xf1, 0x04, 0xb0, 0x65, 0x97, 0x01, 0x1d, 0xf7, 0x9c, 0xf0, 0xfb, 0x03, 0x28, 0x2a, 0x13, 0xd0,
  0xfb, 0xcd, 0x8b, 0xfe, 0x1f, 0x5e, 0x1c, 0x38, 0x6c, 0xc2, 0xe3, 0xa7, 0x2e, 0x2c, 0xcd, 0x1e, 0x44, 0x98, 0x62, 0x2c, 0x9f, 0x30, 0x39, 0x52, 0x6e, 0x41,
  0xe6, 0x56, 0x3d, 0x16, 0x65, 0xf0, 0xd8, 0xb9, 0xd7, 0x88, 0x0d, 0x39, 0x7a, 0x0c, 0xa8, 0x7d, 0x30, 0x99, 0x19, 0x0e, 0x95, 0x9a, 0x60, 0xb6, 0x30, 0x74,
  0xf5, 0x8c, 0xa0, 0xbc, 0x52, 0xc0, 0x28, 0xdd, 0xcb, 0xd7, 0xed, 0x20, 0x2c, 0xe5, 0xfc, 0xd9, 0xa9, 0x1a, 0x56, 0xdd, 0x68, 0x45, 0x5c, 0xba, 0x16, 0xf9,
  0xa5, 0x66, 0xb4, 0xdd, 0x72, 0xa0, 0x7f, 0xc0, 0x0b, 0x50, 0x5c, 0x6b, 0xbb, 0x1d, 0xf5, 0x97, 0xad, 0xb2, 0x7e, 0xb3, 0xc3, 0x81, 0x96, 0x5b, 0x76, 0x59,
  0x3f, 0x5e, 0x69, 0xc1, 0xe4, 0x98, 0x60, 0x7e, 0x0a, 0xed, 0xe7, 0xd4, 0x39, 0x11, 0xe7, 0x29, 0x8e, 0xb1, 0x00, 0x46, 0x47, 0x47, 0x21, 0x50, 0xbf, 0x36,
  0xca, 0x2d, 0x2c, 0x31, 0x41, 0x6f, 0x64, 0xb0, 0x88, 0x0c, 0x05, 0xa4, 0x7f, 0xa1, 0x39, 0x2c, 0x56, 0x3f, 0x76, 0xec, 0x37, 0x22, 0x76, 0x99, 0x56, 0xc2,
  0x80, 0xa5, 0xad, 0xd5, 0xb1, 0x33, 0x84, 0x79, 0x17, 0x61, 0xe7, 0x72, 0xfb, 0x71, 0xe1, 0x8a, 0x0d, 0x07, 0x8b, 0x4d, 0xa8, 0x6d, 0x14, 0xa1, 0x1b, 0xf4,
  0xc1, 0x47, 0x35, 0xa5, 0xda, 0x4d, 0x2d, 0x36, 0xb9, 0xf7, 0xfd, 0x27, 0x4e, 0xd4, 0xd2, 0x2e, 0xf6, 0x15, 0x99, 0x11, 0x12, 0xa3, 0x62, 0x8a, 0xe5, 0x3c,
  0xab, 0x23, 0xff, 0xb3, 0x17, 0x2e, 0xb9, 0xf7, 0xf7, 0x7e, 0x2f, 0x0c, 0x26, 0x26, 0xeb, 0x16, 0x91, 0xea, 0x91, 0xaf, 0x82, 0x76, 0xef, 0x1c, 0xf2, 0x43,
  0xab, 0xf3, 0xc9, 0xb5, 0xbb, 0xba, 0x3d, 0x58, 0x4f, 0x18, 0x87, 0xc4, 0xaa, 0x59, 0x64, 0x82, 0x9a, 0x9d, 0xac, 0x15, 0x91, 0x9b, 0x6f, 0xc4, 0xab, 0x37,
  0xc3, 0x50, 0x6b, 0x7c, 0xe8, 0xfe, 0x34, 0x82, 0xf6, 0x3b, 0x0e, 0x14, 0x50, 0xec, 0xd5, 0x6b, 0x36, 0x38, 0x9c, 0x7e, 0xd8, 0x1c, 0x7e, 0x28, 0x4f, 0x0a,
  0xe8, 0x7c, 0xe4, 0xc4, 0xc7, 0x4f, 0x1e, 0xcc, 0x25, 0x0e, 0x84, 0x25, 0x04, 0x39, 0xb9, 0x25, 0xd7, 0x88, 0xfc, 0x12, 0x33, 0xa2, 0x93, 0xd5, 0xc8, 0x2b,
  0x32, 0xe1, 0xc4, 0x69, 0x01, 0xcd, 0xad, 0x76, 0xf4, 0xf6, 0x79, 0xe4, 0xff, 0xe0, 0xa9, 0xaf, 0xb2, 0x42, 0x40, 0x53, 0xb3, 0x0d, 0x43, 0x2e, 0xe2, 0x04,
  0xed, 0x34, 0x74, 0x0c, 0x7f, 0xe2, 0x11, 0x9b, 0x4e, 0x79, 0xe5, 0xd5, 0x22, 0xb2, 0x88, 0x1b, 0xcf, 0x5f, 0xba, 0xe5, 0x39, 0x25, 0x11, 0x09, 0xab, 0x07,
  0x34, 0xef, 0x41, 0xda, 0xfb, 0x3d, 0xea, 0xeb, 0x72, 0x07, 0x70, 0xf5, 0xba, 0x0d, 0x33, 0x92, 0xd5, 0xbf, 0xf3, 0x8f, 0xb8, 0x34, 0x7f, 0xb1, 0x16, 0x89,
  0x2b, 0x79, 0x64, 0x6c, 0x1a, 0x44, 0x53, 0x9b, 0x1d, 0x65, 0x55, 0x22, 0x76, 0x12, 0xce, 0x35, 0x0d, 0x22, 0x3e, 0xf4, 0x78, 0xd0, 0xfd, 0xd9, 0x43, 0x5c,
  0x14, 0x30, 0x23, 0xe9, 0x1f, 0xfc, 0x27, 0x9b, 0x54, 0x87, 0xf8, 0x28, 0xff, 0xdb, 0x94, 0xf9, 0x2a, 0x24, 0x10, 0x4f, 0xf3, 0x68, 0xae, 0x8d, 0xf4, 0x1e,
  0xe6, 0x11, 0x3f, 0x24, 0x7b, 0x78, 0xe2, 0xaf, 0x5c, 0xfc, 0xf1, 0x06, 0xff, 0x12, 0x69, 0xbf, 0x53, 0x68, 0x47, 0x21, 0x84, 0x91, 0xa4, 0x4f, 0xf4, 0x8d,
  0xbd, 0x63, 0x2f, 0xf7, 0x9f, 0xe7, 0x27, 0x40, 0xbb, 0x5a, 0x53, 0x7e, 0x04, 0x00, 0x00
};
const int favicon_ico_gz_len = 821;



void resetOtaState() {
  otaProgress = -2;  // Default "idle" or "not started" state
  otaStatusMessage = "";
  otaDisplayIpAddress[0] = '\0';
  webOtaActive = false;         // Ensure this is reset
  basicOtaStarted = false;      // Ensure this is reset
  otaIsUndoingChanges = false;  // Reset this flag too
  // availableSdFirmwareCount is reset by prepareSdFirmwareList
}

bool loadCurrentFirmwareInfo() {
  Serial.println("OTA: Loading current firmware info...");
  if (!parseFirmwareMetadataFile(CURRENT_FIRMWARE_INFO_FILENAME, currentRunningFirmware)) {
    Serial.println("OTA: Could not load or parse current firmware info. Assuming unknown.");
    currentRunningFirmware.isValid = false;
    strlcpy(currentRunningFirmware.version, "Unknown", FW_VERSION_MAX_LEN);
    currentRunningFirmware.checksum_md5[0] = '\0';
    return false;
  }
  Serial.printf("OTA: Current running firmware: %s, Checksum: %s\n",
                currentRunningFirmware.version, currentRunningFirmware.checksum_md5);
  return true;
}

bool saveCurrentFirmwareInfo(const FirmwareInfo& infoToSave) {
  if (!isSdCardAvailable()) {
    Serial.println("OTA: SD not available, cannot save current firmware info.");
    return false;
  }
  if (!SD.exists(SYSTEM_INFO_DIR_PATH)) {
    if (!createSdDir(SYSTEM_INFO_DIR_PATH)) {
      Serial.printf("OTA: Failed to create directory %s\n", SYSTEM_INFO_DIR_PATH);
      return false;
    }
  }
  FirmwareInfo toSave = infoToSave;
  toSave.isValid = true;
  if (!saveFirmwareMetadataFile(CURRENT_FIRMWARE_INFO_FILENAME, toSave)) {
    Serial.println("OTA: Failed to save current firmware info.");
    return false;
  }
  Serial.println("OTA: Current firmware info saved.");
  currentRunningFirmware = toSave;
  return true;
}

void setupOtaManager() {
  resetOtaState();
}


// --- Web OTA Implementation (Revised) ---
void handleWebUpdateEnd() {
  otaWebServer.sendHeader("Connection", "close");
  if (Update.hasError() || otaProgress == -1) {  // Check for our internal error flag too
    String errMsg = otaStatusMessage;
    if (errMsg.isEmpty() && Update.hasError()) errMsg = Update.errorString();
    if (errMsg.isEmpty()) errMsg = "Unknown update error.";

    otaWebServer.send(502, "text/plain", errMsg.c_str());
    otaStatusMessage = String("Error: ") + errMsg;
    otaProgress = -1;  // Ensure it's set to error
    Serial.println(otaStatusMessage);

    String tempBinPath = String(FIRMWARE_DIR_PATH) + "/web_upload_pending.bin";
    if (SD.exists(tempBinPath)) {
      SD.remove(tempBinPath);
    }
    // No KFW to remove here as it's only created on success
  } else if (otaProgress == -3) {                                                             // Special case for "already up to date"
    otaWebServer.send(200, "text/html", "Firmware is already up to date. No action taken.");  // Send a success-like code
    otaStatusMessage = "Already up to date.";                                                 // Keep this status for UI
                                                                                              // otaProgress remains -3, UI should show message and allow back.
  } else {                                                                                    // Success (otaProgress should be 100)
    otaWebServer.sendHeader("Refresh", "15; url=/");                                          // Give more time for client to see message
    otaWebServer.sendHeader("Location", "/");
    otaWebServer.send(200, "text/plain", "Update successful! Device is rebooting...");
    // otaStatusMessage is already "Success! Rebooting..." from UPLOAD_FILE_END success path
    // KivaMain loop will handle reboot when otaProgress is 100.
  }
}

void handleWebUpdateUpload() {
  HTTPUpload& upload = otaWebServer.upload();
  static File tempFwFile;
  static FirmwareInfo pendingWebFwInfo;
  static bool upload_error_this_session = false;  // Tracks error within the current upload attempt

  if (upload.status == UPLOAD_FILE_START) {
    upload_error_this_session = false;  // Reset for a new upload
    Serial.printf("WebOTA: Receiving Update (HTTP): %s, Expected Size: %s\n", upload.filename.c_str(), otaWebServer.arg("size").c_str());
    otaProgress = 0;
    otaStatusMessage = "Receiving file...";

    String tempBinPath = String(FIRMWARE_DIR_PATH) + "/web_upload_pending.bin";
    String tempKfwPath = String(FIRMWARE_DIR_PATH) + "/web_upload_pending.kfw";
    if (SD.exists(tempBinPath)) SD.remove(tempBinPath);
    if (SD.exists(tempKfwPath)) SD.remove(tempKfwPath);

    tempFwFile = SD.open(tempBinPath, FILE_WRITE);
    if (!tempFwFile) {
      otaStatusMessage = "Error: SD: Cannot create temp file.";
      otaProgress = -1;
      upload_error_this_session = true;
      Serial.println(otaStatusMessage);
      return;  // WebServer calls handleWebUpdateEnd next
    }
    pendingWebFwInfo = FirmwareInfo();

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (upload_error_this_session) {
      return;
    }
    if (tempFwFile) {
      size_t bytesWritten = tempFwFile.write(upload.buf, upload.currentSize);
      if (bytesWritten != upload.currentSize) {
        otaStatusMessage = "Error: SD: Write failed to temp file.";
        otaProgress = -1;
        upload_error_this_session = true;
        Serial.println(otaStatusMessage);
        if (tempFwFile) tempFwFile.close();
        return;
      } else {
        if (otaWebServer.hasArg("size")) {
          size_t totalSize = otaWebServer.arg("size").toInt();
          if (totalSize > 0) {
            otaProgress = (tempFwFile.position() * 100) / totalSize;
          } else {
            otaProgress = (otaProgress < 80) ? otaProgress + 2 : 80;
          }
        } else {
          otaProgress = (otaProgress < 80) ? otaProgress + 2 : 80;
        }
      }
    } else {
      otaStatusMessage = "Error: Temp file invalid during write.";
      otaProgress = -1;
      upload_error_this_session = true;
      Serial.println(otaStatusMessage);
      return;
    }
    yield();

  } else if (upload.status == UPLOAD_FILE_END) {
    if (tempFwFile && tempFwFile.operator bool()) {
      tempFwFile.close();
    }

    if (upload_error_this_session || otaProgress == -1) {
      Serial.println("WebOTA: UPLOAD_FILE_END reached with prior error state.");
      return;
    }

    otaStatusMessage = "Verifying upload...";
    otaProgress = 85;
    yield();

    String uploadedFilePath = String(FIRMWARE_DIR_PATH) + "/web_upload_pending.bin";
    String uploadedFileChecksum = calculateFileMD5(SD, uploadedFilePath);

    if (uploadedFileChecksum.isEmpty()) {
      otaStatusMessage = "Error: MD5 calculation failed.";
      otaProgress = -1;
      upload_error_this_session = true;
      Serial.println(otaStatusMessage);
      return;
    }
    uploadedFileChecksum.toLowerCase();

    Serial.printf("WebOTA: Uploaded file MD5: %s\n", uploadedFileChecksum.c_str());
    Serial.printf("WebOTA: Current running FW MD5: %s\n", currentRunningFirmware.checksum_md5);

    if (currentRunningFirmware.isValid && uploadedFileChecksum.equals(currentRunningFirmware.checksum_md5)) {
      otaStatusMessage = "Already running this version.";
      otaProgress = -3;
      Serial.println(otaStatusMessage);
      if (SD.exists(uploadedFilePath)) SD.remove(uploadedFilePath);
      return;
    }

    pendingWebFwInfo.isValid = true;
    String clientTimestampStr = otaWebServer.arg("client_timestamp");  // Try "client_timestamp" from form data
    if (clientTimestampStr.isEmpty()) {
      clientTimestampStr = otaWebServer.arg("ts");  // Fallback to "ts" from query param
    }
    bool clientTimeUsed = false;

    if (clientTimestampStr.length() > 0) {
      int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
      if (sscanf(clientTimestampStr.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
        if (year > 2020 && year < 2100 && month >= 1 && month <= 12 && day >= 1 && day <= 31 && hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
          snprintf(pendingWebFwInfo.version, FW_VERSION_MAX_LEN, "web-%04d%02d%02d-%02d%02d%02d",  // Added seconds to version for more uniqueness
                   year, month, day, hour, minute, second);
          strlcpy(pendingWebFwInfo.build_date, clientTimestampStr.c_str(), FW_BUILD_DATE_MAX_LEN);
          clientTimeUsed = true;
          Serial.printf("WebOTA: Using client-provided timestamp for metadata: %s\n", clientTimestampStr.c_str());
        } else {
          Serial.printf("WebOTA: Client timestamp '%s' parsed but values out of range. Falling back.\n", clientTimestampStr.c_str());
        }
      } else {
        Serial.printf("WebOTA: Failed to parse client timestamp: %s. Falling back.\n", clientTimestampStr.c_str());
      }
    }

    if (!clientTimeUsed) {
      time_t now_ts;
      time(&now_ts);
      struct tm* p_tm = localtime(&now_ts);
      // Check if system time is valid (not epoch)
      if (p_tm->tm_year + 1900 < 2023) {  // If time is clearly epoch or uninitialized
        Serial.println("WebOTA: System time appears to be epoch/uninitialized. Using placeholder for version/date.");
        // Use a generic placeholder version if time is bad
        char placeholderVersion[FW_VERSION_MAX_LEN];
        uint32_t pseudo_random_id = esp_random() % 10000;  // Get a pseudo-random number
        snprintf(placeholderVersion, FW_VERSION_MAX_LEN, "web-notime-%04u", pseudo_random_id);
        strlcpy(pendingWebFwInfo.version, placeholderVersion, FW_VERSION_MAX_LEN);
        strcpy(pendingWebFwInfo.build_date, "N/A - Time Unset");
      } else {
        snprintf(pendingWebFwInfo.version, FW_VERSION_MAX_LEN, "web-%04d%02d%02d-%02d%02d%02d",  // Added seconds
                 p_tm->tm_year + 1900, p_tm->tm_mon + 1, p_tm->tm_mday, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
        strftime(pendingWebFwInfo.build_date, FW_BUILD_DATE_MAX_LEN, "%Y-%m-%dT%H:%M:%SZ", p_tm);
      }
      Serial.println("WebOTA: Using system time for metadata (may be epoch if NTP not synced).");
    }

    strlcpy(pendingWebFwInfo.checksum_md5, uploadedFileChecksum.c_str(), FW_CHECKSUM_MD5_MAX_LEN);

    String finalBinName = String(pendingWebFwInfo.version) + ".bin";
    String finalKfwName = String(pendingWebFwInfo.version) + FIRMWARE_METADATA_EXTENSION;
    strlcpy(pendingWebFwInfo.binary_filename, finalBinName.c_str(), FW_BINARY_FILENAME_MAX_LEN);

    String descDatePart = pendingWebFwInfo.build_date;
    if (strcmp(pendingWebFwInfo.build_date, "N/A - Time Unset") != 0) {
      int tPos = descDatePart.indexOf('T');
      int zPos = descDatePart.indexOf('Z');
      int dotPos = descDatePart.indexOf('.');
      if (tPos != -1) {
        int endDescDatePos = -1;
        if (zPos > tPos) endDescDatePos = zPos;
        else if (dotPos > tPos) endDescDatePos = dotPos;
        if (endDescDatePos != -1) descDatePart = descDatePart.substring(0, endDescDatePos);
        else if (descDatePart.length() > 19) descDatePart = descDatePart.substring(0, 19);
      } else if (descDatePart.length() > 19) {
        descDatePart = descDatePart.substring(0, 19);
      }
    }
    snprintf(pendingWebFwInfo.description, FW_DESCRIPTION_MAX_LEN, "Web Upload @ %s", descDatePart.c_str());

    otaStatusMessage = "Starting flash process...";
    otaProgress = 90;
    yield();

    File fwToFlash = SD.open(uploadedFilePath, FILE_READ);
    if (!fwToFlash) {
      otaStatusMessage = "Error: SD: Cannot reopen temp file for flashing.";
      otaProgress = -1;
      upload_error_this_session = true;
      Serial.println(otaStatusMessage);
      return;
    }
    size_t updateSize = fwToFlash.size();

    Update.setMD5(pendingWebFwInfo.checksum_md5);
    if (Update.begin(updateSize)) {
      otaStatusMessage = "Flashing firmware...";
      otaProgress = 95;

      size_t written = Update.writeStream(fwToFlash);
      fwToFlash.close();

      if (written == updateSize) {
        if (Update.end(true)) {
          Serial.printf("WebOTA: Update Success: %u bytes\n", written);
          otaProgress = 100;
          otaStatusMessage = "Success! Rebooting...";

          String finalBinPath = String(FIRMWARE_DIR_PATH) + "/" + finalBinName;
          String finalKfwPath = String(FIRMWARE_DIR_PATH) + "/" + finalKfwName;

          if (SD.exists(finalBinPath)) SD.remove(finalBinPath);
          if (SD.exists(finalKfwPath)) SD.remove(finalKfwPath);

          if (!SD.rename(uploadedFilePath, finalBinPath)) {
            Serial.printf("WebOTA: FAILED to rename %s to %s\n", uploadedFilePath.c_str(), finalBinPath.c_str());
          } else {
            Serial.printf("WebOTA: Renamed %s to %s\n", uploadedFilePath.c_str(), finalBinPath.c_str());
          }
          saveFirmwareMetadataFile(finalKfwPath, pendingWebFwInfo);
          saveCurrentFirmwareInfo(pendingWebFwInfo);

        } else {
          Update.printError(Serial);
          otaStatusMessage = String("Flash End Fail: ") + Update.errorString();
          otaProgress = -1;
        }
      } else {
        Update.printError(Serial);
        otaStatusMessage = String("Flash Write Fail. Wrote ") + String(written) + "/" + String(updateSize);
        otaProgress = -1;
        Update.abort();
      }
    } else {
      Update.printError(Serial);
      otaStatusMessage = String("Flash Begin Fail: ") + Update.errorString();
      otaProgress = -1;
      if (fwToFlash) fwToFlash.close();
    }

  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Serial.println("WebOTA: Upload aborted by client (UPLOAD_FILE_ABORTED status).");
    if (tempFwFile && tempFwFile.operator bool()) {
      tempFwFile.close();
    }
    String tempBinPath = String(FIRMWARE_DIR_PATH) + "/web_upload_pending.bin";
    if (SD.exists(tempBinPath)) {
      SD.remove(tempBinPath);
    }
    Update.abort();
    otaStatusMessage = "Upload aborted by client.";
    otaProgress = -1;
    upload_error_this_session = true;
  }
}

static String generateRandomPassword(uint8_t len) {
  String pass = "";
  const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  for (uint8_t i = 0; i < len; ++i) {
    pass += alphanum[random(0, sizeof(alphanum) - 1)];
  }
  return pass;
}

bool startWebOtaUpdate() {
  if (webOtaActive || isJammingOperationActive) {
    otaStatusMessage = webOtaActive ? "Web OTA already active." : "Jamming active.";
    Serial.println("OTA Manager: " + otaStatusMessage);
    return false;
  }
  resetOtaState();

  Serial.println("OTA Manager: Attempting to start Web OTA (AP Mode)...");

  setWifiHardwareState(true, WIFI_MODE_AP);
  delay(300);

  if (!wifiHardwareEnabled || WiFi.getMode() != WIFI_MODE_AP) {
    otaStatusMessage = "Failed to init AP Mode. Check logs.";
    if (wifiHardwareEnabled && WiFi.getMode() != WIFI_MODE_AP) {
      otaStatusMessage = "Wi-Fi not in AP Mode. Current: " + String(WiFi.getMode());
    } else if (!wifiHardwareEnabled) {
      otaStatusMessage = "Wi-Fi HW failed to enable for AP.";
    }
    Serial.println("OTA Manager: " + otaStatusMessage);
    otaProgress = -1;
    return false;
  }
  Serial.println("OTA Manager: Wi-Fi hardware should now be enabled in AP mode.");

  if (!isSdCardAvailable()) {
    otaStatusMessage = "SD Card Required for OTA Page/Config!";
    Serial.println("OTA Manager: " + otaStatusMessage);
    otaProgress = -1;
    setWifiHardwareState(false);
    return false;
  }
  if (!SD.exists(FIRMWARE_DIR_PATH)) {
    if (!createSdDir(FIRMWARE_DIR_PATH)) {
      otaStatusMessage = "Create FW Dir Fail";
      Serial.println("OTA Manager: " + otaStatusMessage);
      otaProgress = -1;
      setWifiHardwareState(false);
      return false;
    }
  }

  char ap_ssid[32];
  snprintf(ap_ssid, sizeof(ap_ssid), "%s-OTA", OTA_HOSTNAME);
  String ap_password_to_use = "";

  // --- Try to load pre-set AP password ---
  if (SD.exists(OTA_AP_PASSWORD_FILE)) {
    File pwFile = SD.open(OTA_AP_PASSWORD_FILE, FILE_READ);
    if (pwFile) {
      if (pwFile.size() > 0 && pwFile.size() < MAX_AP_PASSWORD_LEN + 5) {  // +5 for potential whitespace/newlines
        String loaded_pw = pwFile.readStringUntil('\n');                   // Read first line
        loaded_pw.trim();                                                  // Remove leading/trailing whitespace including \r
        if (loaded_pw.length() >= MIN_AP_PASSWORD_LEN && loaded_pw.length() <= MAX_AP_PASSWORD_LEN) {
          ap_password_to_use = loaded_pw;
          Serial.printf("OTA Manager: Using pre-set AP password from %s\n", OTA_AP_PASSWORD_FILE);
        } else {
          Serial.printf("OTA Manager: Password in %s is invalid length (%d). Generating random.\n", OTA_AP_PASSWORD_FILE, loaded_pw.length());
        }
      } else if (pwFile.size() > 0) {  // File exists but content might be too long or weird
        Serial.printf("OTA Manager: Content of %s seems invalid (size: %d). Generating random.\n", OTA_AP_PASSWORD_FILE, pwFile.size());
      }
      pwFile.close();
    } else {
      Serial.printf("OTA Manager: Could not open %s. Generating random password.\n", OTA_AP_PASSWORD_FILE);
    }
  } else {
    Serial.printf("OTA Manager: %s not found. Generating random password.\n", OTA_AP_PASSWORD_FILE);
  }

  if (ap_password_to_use.isEmpty()) {  // If no valid pre-set password was loaded
    ap_password_to_use = generateRandomPassword(8);
    Serial.println("OTA Manager: Using randomly generated AP password.");
  }
  // --- End of password logic ---

  if (WiFi.softAP(ap_ssid, ap_password_to_use.c_str())) {
    Serial.printf("AP Configured. SSID: %s, PW: %s, IP: %s\n", ap_ssid, ap_password_to_use.c_str(), WiFi.softAPIP().toString().c_str());

    String displayInfo = WiFi.softAPIP().toString() + " P: " + ap_password_to_use;
    strncpy(otaDisplayIpAddress, displayInfo.c_str(), sizeof(otaDisplayIpAddress) - 1);
    otaDisplayIpAddress[sizeof(otaDisplayIpAddress) - 1] = '\0';

    MDNS.end();
    if (MDNS.begin(OTA_HOSTNAME)) {
      MDNS.addService("http", "tcp", OTA_WEB_SERVER_PORT);
      Serial.printf("MDNS started for AP: %s.local or %s\n", OTA_HOSTNAME, WiFi.softAPIP().toString().c_str());
    } else {
      Serial.println("Error setting up MDNS for AP!");
    }
    otaStatusMessage = String(OTA_HOSTNAME) + ".local";

    otaWebServer.on("/", HTTP_GET, []() {
      if (!isSdCardAvailable()) {
        otaWebServer.send(503, "text/plain", "SD Card Error: Cannot serve OTA page.");
        return;
      }
      File pageFile = SD.open(OTA_HTML_PAGE_PATH, FILE_READ);
      if (!pageFile || pageFile.isDirectory()) {
        Serial.printf("OTA Page: Failed to open '%s' from SD card.\n", OTA_HTML_PAGE_PATH);
        String errorMsg = "<html><title>Error</title><body><h1>OTA Page Not Found</h1><p>Could not load OTA interface. Ensure <code>";
        errorMsg += OTA_HTML_PAGE_PATH;
        errorMsg += "</code> exists on SD card.</p></body></html>";
        otaWebServer.send(404, "text/html", errorMsg);
        if (pageFile) pageFile.close();
        return;
      }
      otaWebServer.streamFile(pageFile, "text/html");
      pageFile.close();
    });

    otaWebServer.on("/favicon.ico", HTTP_GET, []() {
      otaWebServer.sendHeader("Content-Encoding", "gzip");
      otaWebServer.send_P(200, "image/x-icon", favicon_ico_gz, favicon_ico_gz_len);
    });

    otaWebServer.on("/update", HTTP_POST, handleWebUpdateEnd, handleWebUpdateUpload);
    otaWebServer.begin();
    webOtaActive = true;
    otaProgress = 0;
    Serial.println("WebOTA: Server started and listening.");
    return true;
  } else {
    Serial.println("AP Mode Failed (WiFi.softAP call returned false). ESP Wi-Fi State may be inconsistent.");
    otaStatusMessage = "AP Config Fail";
    otaDisplayIpAddress[0] = '\0';
    setWifiHardwareState(false);
    otaProgress = -1;
    webOtaActive = false;
    return false;
  }
}

void handleWebOtaClient() {  // Unchanged
  if (webOtaActive) {
    otaWebServer.handleClient();
  }
}

void stopWebOtaUpdate() {  // Unchanged
  if (!webOtaActive || otaIsUndoingChanges) return;
  otaIsUndoingChanges = true;
  Serial.println("WebOTA: Stopping server and AP mode...");
  MDNS.end();
  otaWebServer.stop();
  WiFi.softAPdisconnect(true);
  setWifiHardwareState(false);
  webOtaActive = false;
  resetOtaState();
  otaIsUndoingChanges = false;
  Serial.println("WebOTA: Stopped.");
}

// --- SD OTA Implementation (Revised) ---
bool prepareSdFirmwareList() {
  if (!isSdCardAvailable()) {
    otaStatusMessage = "SD Card not found.";
    availableSdFirmwareCount = 0;
    return false;
  }

  File root = SD.open(FIRMWARE_DIR_PATH);
  if (!root) {
    otaStatusMessage = "FW dir not found on SD.";
    availableSdFirmwareCount = 0;
    return false;
  }
  if (!root.isDirectory()) {
    otaStatusMessage = "FW path is not a dir.";
    availableSdFirmwareCount = 0;
    root.close();
    return false;
  }

  availableSdFirmwareCount = 0;
  File file = root.openNextFile();
  while (file && availableSdFirmwareCount < MAX_FIRMWARES_ON_SD) {
    String fileName = file.name();
    if (!file.isDirectory() && fileName.endsWith(FIRMWARE_METADATA_EXTENSION)) {
      FirmwareInfo tempInfo;
      // Construct full path for parsing: FIRMWARE_DIR_PATH + "/" + fileName
      String fullMetaPath = String(FIRMWARE_DIR_PATH) + "/" + fileName;
      if (parseFirmwareMetadataFile(fullMetaPath, tempInfo)) {
        // Additionally, check if the corresponding .bin file exists
        String binPath = String(FIRMWARE_DIR_PATH) + "/" + tempInfo.binary_filename;
        if (SD.exists(binPath)) {
          availableSdFirmwares[availableSdFirmwareCount++] = tempInfo;
        } else {
          Serial.printf("OTA: Metadata %s found, but binary %s missing.\n", fileName.c_str(), tempInfo.binary_filename);
        }
      }
    }
    file.close();  // Close current file before opening next
    file = root.openNextFile();
  }
  root.close();

  if (availableSdFirmwareCount == 0) {
    otaStatusMessage = "No firmwares found in " + String(FIRMWARE_DIR_PATH);
  } else {
    otaStatusMessage = String(availableSdFirmwareCount) + " firmware(s) found.";
  }
  Serial.println("OTA: " + otaStatusMessage);
  return availableSdFirmwareCount > 0;
}

void performSdUpdate(const FirmwareInfo& targetFirmware) {
  resetOtaState();                           // Start with a clean OTA state
  otaProgress = 5;                           // Initial "Starting" phase
  otaStatusMessage = "Preparing Update...";  // Initial message for the UI
  // The main loop should immediately draw the OTA_SD_STATUS screen with this message.
  // To make it visible for a moment before the blocking operations:
  // One way is to have the main loop call drawUI() and then proceed.
  // Another way, if this function is called directly from menu selection,
  // the UI update will happen in the next main loop iteration.
  // For a guaranteed display, a small delay here AFTER setting status,
  // combined with a drawUI call in the main loop before this long function fully executes,
  // would be needed. However, this complicates the main loop structure.

  // Let's assume the UI will pick up this "Preparing..." message in the next draw cycle.

  if (isJammingOperationActive) {
    otaStatusMessage = "Jamming active. Cannot update.";
    otaProgress = -1;
    return;  // Early exit, status screen will show this error.
  }

  if (!targetFirmware.isValid) {
    otaStatusMessage = "Invalid target firmware information provided.";
    otaProgress = -1;
    return;  // Status screen will show this.
  }

  Serial.printf("SD_OTA: Attempting update to version: %s, File: %s, Checksum: %s\n",
                targetFirmware.version, targetFirmware.binary_filename, targetFirmware.checksum_md5);
  // Optional: Update status message after initial checks
  otaStatusMessage = "Checking: " + String(targetFirmware.binary_filename);
  otaProgress = 10;
  // Again, rely on main loop to draw this.

  if (currentRunningFirmware.isValid && strcmp(targetFirmware.checksum_md5, currentRunningFirmware.checksum_md5) == 0) {
    otaStatusMessage = "Already running this version.";
    otaProgress = -3;  // Use -3 for "already up to date" as with web OTA
    Serial.println("SD_OTA: " + otaStatusMessage);
    return;
  }

  String binaryPath = String(FIRMWARE_DIR_PATH) + "/" + targetFirmware.binary_filename;
  // otaStatusMessage = String("Opening: ") + targetFirmware.binary_filename; // Already covered by "Checking"
  // otaProgress = 10; // Already set

  File updateBin = SD.open(binaryPath, FILE_READ);
  if (!updateBin || updateBin.isDirectory()) {
    otaStatusMessage = String("Error: Cannot open '") + targetFirmware.binary_filename + "'";
    otaProgress = -1;
    Serial.println("SD_OTA: " + otaStatusMessage);
    if (updateBin) updateBin.close();
    return;
  }

  size_t updateSize = updateBin.size();
  if (updateSize == 0) {
    otaStatusMessage = String("'") + targetFirmware.binary_filename + "' is empty.";
    otaProgress = -1;
    Serial.println("SD_OTA: " + otaStatusMessage);
    updateBin.close();
    return;
  }

  otaStatusMessage = "Verifying & Starting Update...";
  otaProgress = 15;
  Update.setMD5(targetFirmware.checksum_md5);

  if (Update.begin(updateSize)) {
    otaStatusMessage = "Flashing firmware...";
    otaProgress = 25;  // Start of the actual write

    size_t written = Update.writeStream(updateBin);
    // For SD card, progress during writeStream isn't easily granular without custom chunking.
    // So we update progress after the (blocking) write.
    if (written == updateSize) {
      otaProgress = 75;  // Assume most of the way done after successful write
    } else {
      // Partial write before error, estimate progress
      otaProgress = 25 + (int)(((float)written / (float)updateSize) * 50.0f);  // Map to 25-75 range
    }

    if (written == updateSize) {
      otaStatusMessage = "Finalizing update...";
      otaProgress = 85;
      if (Update.end(true)) {
        otaProgress = 100;
        otaStatusMessage = "Success! Rebooting...";
        Serial.println("SD_OTA: Update successful. Saving current FW info.");
        saveCurrentFirmwareInfo(targetFirmware);
      } else {
        otaStatusMessage = String("Finalize error: #") + String(Update.getError()) + " " + Update.errorString();
        otaProgress = -1;
        Serial.println("SD_OTA: " + otaStatusMessage);
      }
    } else {
      otaStatusMessage = String("Write error: ") + String(written) + "/" + String(updateSize);
      otaProgress = -1;
      Serial.println("SD_OTA: " + otaStatusMessage);
      Update.abort();
    }
  } else {
    otaStatusMessage = String("Begin error: #") + String(Update.getError()) + String(" Details: ") + Update.errorString();
    otaProgress = -1;
    Serial.println("SD_OTA: " + otaStatusMessage);
  }
  updateBin.close();
}

// --- Basic OTA (IDE) Implementation ---
void configureArduinoOta() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  // ArduinoOTA.setPassword("your_ota_password"); // Optional

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS - Kiva doesn't use SPIFFS for user data yet
      type = "filesystem";
      // if (SPIFFS.begin(false)) { SPIFFS.end(); } // Unmount SPIFFS if used
    }
    otaStatusMessage = String("Updating ") + type + "...";
    otaProgress = 0;
    Serial.println("BasicOTA: " + otaStatusMessage);
    otaIsUndoingChanges = true;  // Signal that an OTA operation is critically active
  });

  ArduinoOTA.onEnd([]() {
    otaProgress = 100;
    otaStatusMessage = "Success! Rebooting...";
    Serial.println("\nBasicOTA: End. Assuming success and rebooting.");

    FirmwareInfo basicUpdateInfo;
    strlcpy(basicUpdateInfo.version, "BasicOTA Update", FW_VERSION_MAX_LEN);
    time_t now_ts;
    time(&now_ts);
    struct tm* p_tm = localtime(&now_ts);
    strftime(basicUpdateInfo.build_date, FW_BUILD_DATE_MAX_LEN, "%Y-%m-%dT%H:%M:%SZ", p_tm);

    // We cannot easily get the MD5 of the newly flashed image here without complex flash reading.
    // So, we mark it as unknown or a placeholder.
    strlcpy(basicUpdateInfo.checksum_md5, "UNKNOWN_AFTER_IDE_OTA", FW_CHECKSUM_MD5_MAX_LEN);
    strlcpy(basicUpdateInfo.binary_filename, "ide_upload.bin", FW_BINARY_FILENAME_MAX_LEN);  // Placeholder
    strlcpy(basicUpdateInfo.description, "Updated via Arduino IDE", FW_DESCRIPTION_MAX_LEN);
    basicUpdateInfo.isValid = true;
    saveCurrentFirmwareInfo(basicUpdateInfo);
    // Reboot will be handled by KivaMain loop based on otaProgress == 100
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    otaProgress = (progress / (total / 100));
    otaStatusMessage = String("Progress: ") + String(otaProgress) + "%";
    // Serial.printf("BasicOTA: Progress: %u%%\r", otaProgress); // Avoid Serial print in callback for speed
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaProgress = -1;  // Signal error
    String errorMsgBase = "Error[" + String(error) + "]: ";
    String specificError = "";
    if (error == OTA_AUTH_ERROR) specificError = "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) specificError = "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) specificError = "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) specificError = "Receive Failed";
    else if (error == OTA_END_ERROR) specificError = "End Failed";
    else specificError = "Unknown Error";

    otaStatusMessage = errorMsgBase + specificError;
    Serial.println("BasicOTA: " + otaStatusMessage);
    otaIsUndoingChanges = false;  // Allow trying again or backing out
  });
}

bool startBasicOtaUpdate() {
  if (basicOtaStarted || isJammingOperationActive) {
    otaStatusMessage = basicOtaStarted ? "Basic OTA already active." : "Jamming active.";
    Serial.println("OTA Manager: " + otaStatusMessage); // Keep this log for debugging
    // otaProgress might already be set if basicOtaStarted is true, otherwise -2 (idle)
    return false;
  }
  resetOtaState(); // Clear previous OTA state
  // otaIsUndoingChanges = true; // This flag seems more for critical phases, perhaps not needed here.

  if (!wifiHardwareEnabled) {
    otaStatusMessage = "Wi-Fi is Off. Enabling...";
    Serial.println("OTA Manager: " + otaStatusMessage);
    setWifiHardwareState(true, WIFI_MODE_STA); // Explicitly request STA
    delay(200); // Give time for Wi-Fi to enable
    if (!wifiHardwareEnabled) {
      otaStatusMessage = "Failed to enable Wi-Fi for OTA.";
      otaProgress = -1; // Error state
      Serial.println("OTA Manager: " + otaStatusMessage);
      // otaIsUndoingChanges = false;
      return false;
    }
  }

  // After ensuring Wi-Fi hardware is on, check connection status
  if (WiFi.status() != WL_CONNECTED) {
    otaStatusMessage = "Connect to Wi-Fi (STA) first."; // Clear message for UI
    otaProgress = -1; // Indicate an error/precondition not met
    Serial.println("BasicOTA: " + otaStatusMessage);
    // otaIsUndoingChanges = false;
    // Don't disable Wi-Fi here; let the main loop's auto-disable handle it if appropriate,
    // or user might want to go to Wi-Fi setup.
    return false;
  }

  // If we reach here, Wi-Fi is ON and CONNECTED in STA mode.
  Serial.println("BasicOTA: Starting ArduinoOTA...");
  strncpy(otaDisplayIpAddress, WiFi.localIP().toString().c_str(), sizeof(otaDisplayIpAddress) - 1);
  otaDisplayIpAddress[sizeof(otaDisplayIpAddress) - 1] = '\0';

  configureArduinoOta(); // Ensure this is called before ArduinoOTA.begin()
  ArduinoOTA.begin();

  otaStatusMessage = String("IP: ") + WiFi.localIP().toString(); // Show IP for connection
  // otaStatusMessage = String(OTA_HOSTNAME) + ".local or " + WiFi.localIP().toString(); // Alternative
  otaProgress = 0; // Initial success state for "ready"
  basicOtaStarted = true;
  // otaIsUndoingChanges = false; // Already false or not set
  Serial.println("BasicOTA: Ready. IP: " + WiFi.localIP().toString() + " (Hostname: " + OTA_HOSTNAME + ".local)");
  return true;
}

void handleBasicOta() {  // Unchanged
  if (basicOtaStarted) {
    ArduinoOTA.handle();
  }
}

void stopBasicOtaUpdate() {  // Unchanged
  if (!basicOtaStarted || otaIsUndoingChanges) return;
  otaIsUndoingChanges = true;
  Serial.println("BasicOTA: Stopping (no longer handling).");
  basicOtaStarted = false;
  resetOtaState();
  otaIsUndoingChanges = false;
}