#include "sd_card_manager.h"

static bool sdCardInitialized = false;

bool setupSdCard() {
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card: Mount Failed");
    sdCardInitialized = false;
    return false;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("SD Card: No SD card attached");
    sdCardInitialized = false;
    return false;
  }
  sdCardInitialized = true;

  Serial.print("SD Card: Type: ");
  if (cardType == CARD_MMC) Serial.println("MMC");
  else if (cardType == CARD_SD) Serial.println("SDSC");
  else if (cardType == CARD_SDHC) Serial.println("SDHC");
  else Serial.println("UNKNOWN");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card: Size: %lluMB\n", cardSize);


  Serial.println("SD Card: Checking/Creating standard directories...");
  const char *directoriesToEnsure[] = {
    WIFI_DIR,
    MUSIC_DIR,
    CAPTURES_DIR,
    LOGS_DIR,
    SETTINGS_DIR,
    SYSTEM_INFO_DIR_PATH,
    FIRMWARE_DIR_PATH
  };
  for (const char *dirPath : directoriesToEnsure) {
    if (!SD.exists(dirPath)) {
      Serial.printf("SD Card: Directory %s does not exist. Creating...\n", dirPath);
      if (createSdDir(dirPath)) {
        // Successfully created
      } else {
        Serial.printf("SD Card: Failed to create directory %s.\n", dirPath);
      }
    }
  }

  if (SD.exists(CAPTURES_DIR)) {
    if (!SD.exists(HANDSHAKES_DIR)) {
      Serial.printf("SD Card: Directory %s does not exist. Creating...\n", HANDSHAKES_DIR);
      createSdDir(HANDSHAKES_DIR);
    }
  }

  // --- ADDED: Check for and create beacon_ssids.txt if it doesn't exist ---
  const char *beacon_ssid_file_path = "/system/beacon_ssids.txt";
  if (!SD.exists(beacon_ssid_file_path)) {
    Serial.println("SD Card: Beacon SSID file not found. Creating a blank template...");
    const char *file_content = "";
    writeSdFile(beacon_ssid_file_path, file_content);
  }

  const char *rick_roll_file_path = "/system/rick_roll_ssids.txt";
  if (!SD.exists(rick_roll_file_path)) {
    Serial.println("SD Card: Rick Roll SSID file not found. Creating a template...");
    const char *rick_roll_ssids =
      "We're no strangers to love\n"
      "You know the rules and so do I\n"
      "A full commitment's what I'm...\n"
      "Never gonna give you up\n"
      "Never gonna let you down\n"
      "Never gonna run around\n"
      "and desert you\n";
    writeSdFile(rick_roll_file_path, rick_roll_ssids);
  }
  // --- END OF ADDED BLOCK ---


  Serial.println("SD Card: Standard directory check complete.");
  return sdCardInitialized;
}
bool isSdCardAvailable() {
  return sdCardInitialized;
}

void listDirectory(const char *dirname, uint8_t levels) {
  if (!sdCardInitialized) {
    Serial.println("SD Card not initialized.");
    return;
  }
  Serial.printf("Listing directory: %s\n", dirname);

  File root = SD.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    root.close();
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        // Construct full path for recursive call
        char subDirPath[256];
        snprintf(subDirPath, sizeof(subDirPath), "%s/%s", dirname, file.name());
        // Normalize path (remove double slashes if dirname is "/")
        if (strcmp(dirname, "/") == 0) {
          snprintf(subDirPath, sizeof(subDirPath), "/%s", file.name());
        }
        listDirectory(subDirPath, levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
  root.close();
}

bool createSdDir(const char *path) {
  if (!sdCardInitialized) {
    Serial.println("SD Card not initialized.");
    return false;
  }
  Serial.printf("Creating Dir: %s\n", path);
  if (SD.mkdir(path)) {
    Serial.println("Dir created");
    return true;
  } else {
    Serial.println("mkdir failed");
    return false;
  }
}

bool removeSdDir(const char *path) {
  if (!sdCardInitialized) {
    Serial.println("SD Card not initialized.");
    return false;
  }
  Serial.printf("Removing Dir: %s\n", path);
  if (SD.rmdir(path)) {
    Serial.println("Dir removed");
    return true;
  } else {
    Serial.println("rmdir failed");
    return false;
  }
}

String readSdFile(const char *path) {
  if (!sdCardInitialized) {
    Serial.println("SD Card not initialized.");
    return "";
  }
  Serial.printf("Reading file: %s\n", path);
  String fileContent = "";
  File file = SD.open(path, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return "";
  }

  while (file.available()) {
    fileContent += (char)file.read();
  }
  file.close();
  return fileContent;
}

bool writeSdFile(const char *path, const char *message) {
  if (!sdCardInitialized) {
    Serial.println("SD Card not initialized.");
    return false;
  }
  Serial.printf("Writing file: %s\n", path);

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return false;
  }
  if (file.print(message)) {
    Serial.println("File written");
    file.close();
    return true;
  } else {
    Serial.println("Write failed");
    file.close();
    return false;
  }
}

bool appendSdFile(const char *path, const char *message) {
  if (!sdCardInitialized) {
    Serial.println("SD Card not initialized.");
    return false;
  }
  Serial.printf("Appending to file: %s\n", path);

  File file = SD.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return false;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
    file.close();
    return true;
  } else {
    Serial.println("Append failed");
    file.close();
    return false;
  }
}

bool renameSdFile(const char *path1, const char *path2) {
  if (!sdCardInitialized) {
    Serial.println("SD Card not initialized.");
    return false;
  }
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (SD.rename(path1, path2)) {
    Serial.println("File renamed");
    return true;
  } else {
    Serial.println("Rename failed");
    return false;
  }
}

bool deleteSdFile(const char *path) {
  if (!sdCardInitialized) {
    Serial.println("SD Card not initialized.");
    return false;
  }
  Serial.printf("Deleting file: %s\n", path);
  if (SD.remove(path)) {
    Serial.println("File deleted");
    return true;
  } else {
    Serial.println("Delete failed");
    return false;
  }
}

uint64_t getSdTotalSpaceMB() {
  if (!sdCardInitialized) return 0;
  return SD.totalBytes() / (1024 * 1024);
}

uint64_t getSdUsedSpaceMB() {
  if (!sdCardInitialized) return 0;
  return SD.usedBytes() / (1024 * 1024);
}


void testSdFileIO(const char *path) {
  if (!sdCardInitialized) {
    Serial.println("SD Card not initialized. Cannot run test.");
    return;
  }
  File file = SD.open(path, FILE_READ);  // Changed to FILE_READ for initial check
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read in %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for initial read test or file doesn't exist.");
  }

  file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing test");
    return;
  }

  // Prepare buffer with some data (optional, can be anything)
  for (int k = 0; k < 512; k++) buf[k] = k % 256;

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {  // Write 1MB
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%lu bytes written in %u ms\n", (unsigned long)2048 * 512, end);
  file.close();
  Serial.println("SD I/O Test Complete.");
}