#include "ota_manager.h"
#include "ui_drawing.h"
#include <FS.h>
#include <SPIFFS.h>
#include "sd_card_manager.h"
#include <esp_wifi.h>  // <--- ADD THIS LINE

// Define WebServer HTML and Favicon data from user's example
const char* indexHtml = R"literal(
  <!DOCTYPE html>
  <link rel='icon' href='/favicon.ico' sizes='any'>
  <body style='width:480px'>
    <h2>Kiva Firmware Update</h2>
    <p>Ensure the device remains powered on during the update.</p>
    <form method='POST' enctype='multipart/form-data' id='upload-form'>
      <input type='file' id='file' name='update'>
      <input type='submit' value='Update'>
    </form>
    <br>
    <div id='prg_container' style='width:100%; background-color:#ccc; border-radius: 5px; overflow: hidden;'>
        <div id='prg' style='width:0; background-color:blue; color:white; text-align:center; line-height: 20px; height:20px; border-radius: 5px;'>0%</div>
    </div>
    <p id='status_msg'></p>
  </body>
  <script>
    var prg = document.getElementById('prg');
    var prgContainer = document.getElementById('prg_container');
    var statusMsg = document.getElementById('status_msg');
    var form = document.getElementById('upload-form');

    form.addEventListener('submit', el=>{
      prg.style.backgroundColor = 'blue';
      statusMsg.innerHTML = 'Uploading...';
      el.preventDefault();
      var data = new FormData(form);
      var req = new XMLHttpRequest();
      var fsize = document.getElementById('file').files[0].size;
      req.open('POST', '/update?size=' + fsize);
      req.upload.addEventListener('progress', p=>{
        let w = Math.round(p.loaded/p.total*100) + '%';
          if(p.lengthComputable){
             prg.innerHTML = w;
             prg.style.width = w;
          }
          if(w == '100%') {
            prg.style.backgroundColor = 'green';
            statusMsg.innerHTML = 'Upload complete. Processing update...';
          }
      });
      req.addEventListener('load', function(e) {
        if (req.status == 200) {
            statusMsg.innerHTML = 'Update successful! Device is rebooting... Page will refresh.';
            prg.style.backgroundColor = 'green';
            setTimeout(function(){ window.location.reload(); }, 10000); // Refresh after 10s
        } else if (req.status == 307) { // Redirect, often implies success and reboot
            statusMsg.innerHTML = 'Update successful! Device is rebooting... Page will redirect.';
             prg.style.backgroundColor = 'green';
            // Browser will handle redirect from server.
        }
        else {
            statusMsg.innerHTML = 'Update failed: ' + req.responseText + ' (Status: ' + req.status + ')';
            prg.style.backgroundColor = 'red';
        }
      });
      req.addEventListener('error', function(e) {
        statusMsg.innerHTML = 'Update request failed. Check connection or console.';
        prg.style.backgroundColor = 'red';
      });
      req.send(data);
     });
  </script>
)literal";

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
  static bool upload_error_this_session = false; // Tracks error within the current upload attempt

  if (upload.status == UPLOAD_FILE_START) {
    upload_error_this_session = false; // Reset for a new upload
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
      return; // WebServer calls handleWebUpdateEnd next
    }
    pendingWebFwInfo = FirmwareInfo();

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (upload_error_this_session) { // If an error (like file open) already occurred
      return;
    }

    if (tempFwFile) { // Ensure file object is valid
      size_t bytesWritten = tempFwFile.write(upload.buf, upload.currentSize);
      if (bytesWritten != upload.currentSize) {
        otaStatusMessage = "Error: SD: Write failed to temp file.";
        otaProgress = -1;
        upload_error_this_session = true;
        Serial.println(otaStatusMessage);
        if (tempFwFile) tempFwFile.close(); // Close file on error
        return; // WebServer calls handleWebUpdateEnd
      } else {
        // Progress calculation
        if (otaWebServer.hasArg("size")) {
          size_t totalSize = otaWebServer.arg("size").toInt();
          if (totalSize > 0) {
            otaProgress = (tempFwFile.position() * 100) / totalSize;
          } else { // Fallback if size is 0 or invalid
            otaProgress = (otaProgress < 80) ? otaProgress + 2 : 80; // Slow arbitrary progress
          }
        } else { // Fallback if "size" arg is missing
          otaProgress = (otaProgress < 80) ? otaProgress + 2 : 80; // Slow arbitrary progress
        }
        // UI status message is updated by client-side JS; server side only for major states/errors
      }
    } else { // tempFwFile is not valid - should have been caught by UPLOAD_FILE_START error
      otaStatusMessage = "Error: Temp file invalid during write.";
      otaProgress = -1;
      upload_error_this_session = true;
      Serial.println(otaStatusMessage);
      return;
    }
    yield(); // CRITICAL: Allow other tasks (network) to run

  } else if (upload.status == UPLOAD_FILE_END) {
    if (tempFwFile && tempFwFile.operator bool()) { // Ensure file is valid before closing
      tempFwFile.close();
    }

    if (upload_error_this_session || otaProgress == -1) {
      Serial.println("WebOTA: UPLOAD_FILE_END reached with prior error state.");
      // otaStatusMessage and otaProgress should already reflect the error.
      return; // WebServer calls handleWebUpdateEnd
    }

    otaStatusMessage = "Verifying upload...";
    otaProgress = 85;
    yield(); // Allow UI update and background tasks

    String uploadedFilePath = String(FIRMWARE_DIR_PATH) + "/web_upload_pending.bin";
    String uploadedFileChecksum = calculateFileMD5(SD, uploadedFilePath);

    if (uploadedFileChecksum.isEmpty()) {
      otaStatusMessage = "Error: MD5 calculation failed.";
      otaProgress = -1;
      upload_error_this_session = true; // Mark error for this session
      Serial.println(otaStatusMessage);
      return;
    }
    uploadedFileChecksum.toLowerCase();

    Serial.printf("WebOTA: Uploaded file MD5: %s\n", uploadedFileChecksum.c_str());
    Serial.printf("WebOTA: Current running FW MD5: %s\n", currentRunningFirmware.checksum_md5);

    if (currentRunningFirmware.isValid && uploadedFileChecksum.equals(currentRunningFirmware.checksum_md5)) {
      otaStatusMessage = "Already running this version.";
      otaProgress = -3; // Special code for "already up to date"
      Serial.println(otaStatusMessage);
      if (SD.exists(uploadedFilePath)) SD.remove(uploadedFilePath);
      return;
    }

    // Prepare FirmwareInfo for the uploaded file
    pendingWebFwInfo.isValid = true;
    time_t now_ts;
    time(&now_ts);
    struct tm* p_tm = localtime(&now_ts);
    char tempVersionStr[FW_VERSION_MAX_LEN];
    snprintf(tempVersionStr, FW_VERSION_MAX_LEN, "web-%04d%02d%02d-%02d%02d",
             p_tm->tm_year + 1900, p_tm->tm_mon + 1, p_tm->tm_mday, p_tm->tm_hour, p_tm->tm_min);
    strlcpy(pendingWebFwInfo.version, tempVersionStr, FW_VERSION_MAX_LEN);
    strftime(pendingWebFwInfo.build_date, FW_BUILD_DATE_MAX_LEN, "%Y-%m-%dT%H:%M:%SZ", p_tm);
    strlcpy(pendingWebFwInfo.checksum_md5, uploadedFileChecksum.c_str(), FW_CHECKSUM_MD5_MAX_LEN);

    String finalBinName = String(pendingWebFwInfo.version) + ".bin";
    String finalKfwName = String(pendingWebFwInfo.version) + FIRMWARE_METADATA_EXTENSION;
    strlcpy(pendingWebFwInfo.binary_filename, finalBinName.c_str(), FW_BINARY_FILENAME_MAX_LEN);
    snprintf(pendingWebFwInfo.description, FW_DESCRIPTION_MAX_LEN, "Uploaded via Web @ %s", pendingWebFwInfo.build_date);

    otaStatusMessage = "Starting flash process...";
    otaProgress = 90;
    yield(); // Allow UI update

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
      otaProgress = 95; // Indicative progress for the blocking flash write
      
      size_t written = Update.writeStream(fwToFlash);
      fwToFlash.close(); // Close file after reading

      if (written == updateSize) {
        if (Update.end(true)) { // true to set the boot partition flags
          Serial.printf("WebOTA: Update Success: %u bytes\n", written);
          otaProgress = 100;
          otaStatusMessage = "Success! Rebooting...";

          // Save metadata for the newly flashed firmware
          String finalBinPath = String(FIRMWARE_DIR_PATH) + "/" + finalBinName;
          String finalKfwPath = String(FIRMWARE_DIR_PATH) + "/" + finalKfwName;

          if (SD.exists(finalBinPath)) SD.remove(finalBinPath); // Remove if old one with same name exists
          if (SD.exists(finalKfwPath)) SD.remove(finalKfwPath);

          if (!SD.rename(uploadedFilePath, finalBinPath)) {
            Serial.printf("WebOTA: FAILED to rename %s to %s\n", uploadedFilePath.c_str(), finalBinPath.c_str());
            // Flashing was successful, but the file on SD is still named ...pending.bin
            // This is not ideal but the device will boot the new FW.
          } else {
             Serial.printf("WebOTA: Renamed %s to %s\n", uploadedFilePath.c_str(), finalBinPath.c_str());
          }
          saveFirmwareMetadataFile(finalKfwPath, pendingWebFwInfo);
          saveCurrentFirmwareInfo(pendingWebFwInfo); // Update current_fw.json to reflect new FW

        } else { // Update.end() failed
          Update.printError(Serial);
          otaStatusMessage = String("Flash End Fail: ") + Update.errorString();
          otaProgress = -1;
          // No need to set upload_error_this_session as this is the final step error.
        }
      } else { // written != updateSize (Update.writeStream failed)
        Update.printError(Serial);
        otaStatusMessage = String("Flash Write Fail. Wrote ") + String(written) + "/" + String(updateSize);
        otaProgress = -1;
        Update.abort(); // Important to call abort if writeStream fails
      }
    } else { // Update.begin() failed
      Update.printError(Serial);
      otaStatusMessage = String("Flash Begin Fail: ") + Update.errorString();
      otaProgress = -1;
      if (fwToFlash) fwToFlash.close(); // Ensure file is closed
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
    Update.abort(); // Attempt to abort any Update process state
    otaStatusMessage = "Upload aborted by client."; // This will be shown on UI
    otaProgress = -1;
    upload_error_this_session = true; // Mark that this upload attempt failed
  }
  // handleWebUpdateEnd() will be called by the WebServer library automatically
  // to send the final response to the client based on otaProgress and otaStatusMessage.
}


bool startWebOtaUpdate() {
  if (webOtaActive || isJammingOperationActive) {
    otaStatusMessage = webOtaActive ? "Web OTA already active." : "Jamming active.";
    Serial.println("OTA Manager: " + otaStatusMessage);
    return false;
  }
  resetOtaState();

  Serial.println("OTA Manager: Attempting to start Web OTA (AP Mode)...");

  // 1. Ensure Wi-Fi hardware is ON and in AP mode.
  setWifiHardwareState(true, WIFI_MODE_AP);  // Request AP mode via our robust function
  // A delay here might allow event queue issues to settle, or for AP to fully come up.
  // The delays within setWifiHardwareState after start should help.
  delay(300);  // Additional delay after setWifiHardwareState completes for AP

  if (!wifiHardwareEnabled || WiFi.getMode() != WIFI_MODE_AP) {
    // setWifiHardwareState should have set an appropriate wifiStatusString or failed more clearly
    otaStatusMessage = "Failed to init AP Mode. Check logs.";  // Generic message if setWifiHardwareState didn't provide one
    if (wifiHardwareEnabled && WiFi.getMode() != WIFI_MODE_AP) {
      otaStatusMessage = "Wi-Fi not in AP Mode. Current: " + String(WiFi.getMode());
    } else if (!wifiHardwareEnabled) {
      otaStatusMessage = "Wi-Fi HW failed to enable for AP.";
    }
    Serial.println("OTA Manager: " + otaStatusMessage);
    // setWifiHardwareState(false); // Attempt to turn it off cleanly if full setup failed
    otaProgress = -1;
    return false;
  }
  Serial.println("OTA Manager: Wi-Fi hardware should now be enabled in AP mode.");

  // 2. Proceed with SD card checks and AP configuration (SSID, WebServer)
  if (!isSdCardAvailable()) {
    otaStatusMessage = "SD Card Required";
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

  // By now, esp_wifi should be stopped, deinit, init, mode AP, started.
  // WiFi.softAP() should just configure the running AP.
  if (WiFi.softAP(ap_ssid)) {
    Serial.printf("AP Configured by WiFi.softAP. SSID: %s, IP: %s\n", ap_ssid, WiFi.softAPIP().toString().c_str());
    strncpy(otaDisplayIpAddress, WiFi.softAPIP().toString().c_str(), sizeof(otaDisplayIpAddress) - 1);
    otaDisplayIpAddress[sizeof(otaDisplayIpAddress) - 1] = '\0';

    MDNS.end();
    if (MDNS.begin(OTA_HOSTNAME)) {
      MDNS.addService("http", "tcp", OTA_WEB_SERVER_PORT);
      Serial.printf("MDNS started for AP: %s.local or %s\n", OTA_HOSTNAME, WiFi.softAPIP().toString().c_str());
      otaStatusMessage = String(OTA_HOSTNAME) + ".local";
    } else {
      Serial.println("Error setting up MDNS for AP!");
      otaStatusMessage = WiFi.softAPIP().toString();
    }

    otaWebServer.on("/", HTTP_GET, []() {
      otaWebServer.send(200, "text/html", indexHtml);
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
  if (isJammingOperationActive) {
    otaStatusMessage = "Jamming active. Cannot update.";
    otaProgress = -1;
    return;
  }
  resetOtaState();
  otaProgress = 5;  // Starting phase

  if (!targetFirmware.isValid) {
    otaStatusMessage = "Invalid target firmware information provided.";
    otaProgress = -1;
    return;
  }

  Serial.printf("SD_OTA: Attempting update to version: %s, File: %s, Checksum: %s\n",
                targetFirmware.version, targetFirmware.binary_filename, targetFirmware.checksum_md5);
  Serial.printf("SD_OTA: Current running version: %s, Checksum: %s\n",
                currentRunningFirmware.version, currentRunningFirmware.checksum_md5);

  if (currentRunningFirmware.isValid && strcmp(targetFirmware.checksum_md5, currentRunningFirmware.checksum_md5) == 0) {
    otaStatusMessage = "Already running this version.";
    otaProgress = 100;  // Special state: "done", but no actual update performed. UI should reflect this.
                        // Or use a distinct value like -3 if 100 is strictly for "rebooting".
                        // For now, 100 implies completion of the *action*.
    Serial.println("SD_OTA: " + otaStatusMessage);
    return;
  }

  String binaryPath = String(FIRMWARE_DIR_PATH) + "/" + targetFirmware.binary_filename;
  otaStatusMessage = String("Opening: ") + targetFirmware.binary_filename;
  otaProgress = 10;

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
  Update.setMD5(targetFirmware.checksum_md5);  // Set expected MD5 for verification by Update class

  if (Update.begin(updateSize)) {
    otaStatusMessage = "Writing firmware...";
    otaProgress = 25;

    size_t written = Update.writeStream(updateBin);  // This is blocking
    // After writeStream, update progress before Update.end()
    if (written == updateSize) {
      otaProgress = 75;
    } else {
      otaProgress = map(written, 0, updateSize, 25, 75);  // map progress if write was partial before error
    }

    if (written == updateSize) {
      otaStatusMessage = "Finalizing update...";
      otaProgress = 85;
      if (Update.end(true)) {  // true to set boot partition
        otaProgress = 100;
        otaStatusMessage = "Success! Rebooting...";
        Serial.println("SD_OTA: Update successful. Saving current FW info.");
        saveCurrentFirmwareInfo(targetFirmware);  // Update current_fw.json
                                                  // Reboot will be handled by KivaMain loop based on otaProgress = 100
      } else {                                    // Update.end() failed
        otaStatusMessage = String("Finalize error: #") + String(Update.getError()) + " " + Update.errorString();
        otaProgress = -1;
        Serial.println("SD_OTA: " + otaStatusMessage);
      }
    } else {  // written != updateSize (Update.writeStream failed)
      otaStatusMessage = String("Write error: ") + String(written) + "/" + String(updateSize);
      otaProgress = -1;
      Serial.println("SD_OTA: " + otaStatusMessage);
      Update.abort();  // Abort the update process
    }
  } else {  // Update.begin() failed
    otaStatusMessage = String("Begin error: #") + String(Update.getError()) + String(" Details: ") + Update.errorString();
    otaProgress = -1;
    Serial.println("SD_OTA: " + otaStatusMessage);
  }
  updateBin.close();  // Ensure file is closed in all paths
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

bool startBasicOtaUpdate() {  // Unchanged from previous, relies on onEnd to update current FW info
  if (basicOtaStarted || isJammingOperationActive) {
    otaStatusMessage = basicOtaStarted ? "Basic OTA already active." : "Jamming active.";
    return false;
  }
  resetOtaState();
  otaIsUndoingChanges = true;

  if (!wifiHardwareEnabled) {
    otaStatusMessage = "Wi-Fi is Off. Enabling...";
    setWifiHardwareState(true, WIFI_MODE_STA);  // Explicitly request STA
    delay(200);
    if (!wifiHardwareEnabled) {
      otaStatusMessage = "Failed to enable Wi-Fi.";
      otaProgress = -1;
      otaIsUndoingChanges = false;
      return false;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    otaStatusMessage = "Connect to Wi-Fi STA first.";
    otaProgress = -1;
    Serial.println("BasicOTA: " + otaStatusMessage);
    otaIsUndoingChanges = false;
    return false;
  }

  Serial.println("BasicOTA: Starting ArduinoOTA...");
  strncpy(otaDisplayIpAddress, WiFi.localIP().toString().c_str(), sizeof(otaDisplayIpAddress) - 1);
  otaDisplayIpAddress[sizeof(otaDisplayIpAddress) - 1] = '\0';

  configureArduinoOta();
  ArduinoOTA.begin();

  otaStatusMessage = String(OTA_HOSTNAME) + ".local or " + WiFi.localIP().toString();
  otaProgress = 0;
  basicOtaStarted = true;
  otaIsUndoingChanges = false;
  Serial.println("BasicOTA: Ready. IP: " + WiFi.localIP().toString());
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