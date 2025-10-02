#include "Logger.h"
#include "SdCardManager.h"
#include <vector>
#include <algorithm>
#include <time.h>

// Singleton instance definition
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : isInitialized_(false) {}

void Logger::setup() {
    if (!SdCardManager::getInstance().isAvailable()) {
        Serial.println("[LOGGER] SD Card not available. File logging disabled.");
        isInitialized_ = false;
        return;
    }

    const char* logDir = SD_ROOT::DATA_LOGS;
    if (!SdCardManager::getInstance().exists(logDir)) {
        SdCardManager::getInstance().createDir(logDir);
    }

    manageLogFiles();

    char latestLogPath[64];
    snprintf(latestLogPath, sizeof(latestLogPath), "%s/system_log_latest.txt", logDir);
    currentLogFile_ = latestLogPath;

    isInitialized_ = true;
    Serial.printf("[LOGGER] Logging to new file: %s\n", currentLogFile_.c_str());
    log(LogLevel::INFO, "LOGGER", "--- System Boot ---");
}

void Logger::manageLogFiles() {
    const char* logDir = SD_ROOT::DATA_LOGS;
    const char* logPrefix = "system_log_";
    const char* latestLogName = "system_log_latest.txt";
    const char* logExtension = ".txt";

    char previousLatestPath[64];
    snprintf(previousLatestPath, sizeof(previousLatestPath), "%s/%s", logDir, latestLogName);

    if (SdCardManager::getInstance().exists(previousLatestPath)) {
        time_t now;
        time(&now);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);

        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", &timeinfo);

        char archivePath[64];
        snprintf(archivePath, sizeof(archivePath), "%s/%s%s%s", logDir, logPrefix, timestamp, logExtension);
        
        if (SdCardManager::getInstance().renameFile(previousLatestPath, archivePath)) {
            Serial.printf("[LOGGER] Archived previous log to: %s\n", archivePath);
        } else {
            Serial.printf("[LOGGER] ERROR: Failed to archive %s\n", previousLatestPath);
        }
    }

    std::vector<String> archivedLogs;
    // --- THIS IS THE FIX ---
    File root = SdCardManager::getInstance().openFileUncached(logDir);
    if (!root || !root.isDirectory()) {
        if (root) root.close();
        return;
    }

    File file = root.openNextFile();
    while(file) {
        String fileName = file.name();
        if (!file.isDirectory() && fileName.startsWith(logPrefix) && fileName != latestLogName) {
            archivedLogs.push_back(String(logDir) + "/" + fileName);
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();

    std::sort(archivedLogs.begin(), archivedLogs.end());

    while (!archivedLogs.empty() && archivedLogs.size() >= MAX_LOG_FILES) {
        Serial.printf("[LOGGER] Max log files reached. Deleting oldest archive: %s\n", archivedLogs[0].c_str());
        SdCardManager::getInstance().deleteFile(archivedLogs[0].c_str());
        archivedLogs.erase(archivedLogs.begin());
    }
}