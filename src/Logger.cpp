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

// Constructor is simple, member variables for paths are no longer needed.
Logger::Logger() : isInitialized_(false) {}

void Logger::setup() {
    if (!SdCardManager::isAvailable()) {
        Serial.println("[LOGGER] SD Card not available. File logging disabled.");
        isInitialized_ = false;
        return;
    }

    // Ensure the logs directory exists
    const char* logDir = SD_ROOT::DATA_LOGS;
    if (!SdCardManager::exists(logDir)) {
        SdCardManager::createDir(logDir);
    }

    // Perform the new archive and cleanup logic
    manageLogFiles();

    // The new log file for this session is ALWAYS the "latest" file.
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

    // 1. Archive the previous "latest" log file if it exists
    if (SdCardManager::exists(previousLatestPath)) {
        // --- MODIFICATION: Use a proper timestamp ---
        time_t now;
        time(&now);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);

        char timestamp[20];
        // Format as YYYYMMDD-HHMMSS for easy sorting
        strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", &timeinfo);

        char archivePath[64];
        // Use the new timestamp instead of millis()
        snprintf(archivePath, sizeof(archivePath), "%s/%s%s%s", logDir, logPrefix, timestamp, logExtension);
        // --- END OF MODIFICATION ---
        
        if (SdCardManager::renameFile(previousLatestPath, archivePath)) {
            Serial.printf("[LOGGER] Archived previous log to: %s\n", archivePath);
        } else {
            Serial.printf("[LOGGER] ERROR: Failed to archive %s\n", previousLatestPath);
        }
    }

    // 2. Clean up old archived logs if we exceed the limit
    std::vector<String> archivedLogs;
    File root = SdCardManager::openFile(logDir);
    if (!root || !root.isDirectory()) {
        if (root) root.close();
        return;
    }

    File file = root.openNextFile();
    while(file) {
        String fileName = file.name();
        // An archived log starts with the prefix but IS NOT the "latest" log.
        if (!file.isDirectory() && fileName.startsWith(logPrefix) && fileName != latestLogName) {
            archivedLogs.push_back(String(logDir) + "/" + fileName);
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();

    // Sort files alphabetically, which also sorts them by timestamp in the name.
    std::sort(archivedLogs.begin(), archivedLogs.end());

    // We keep (MAX_LOG_FILES - 1) archives, because one slot is for the "latest" file.
    // The check `archivedLogs.size() >= MAX_LOG_FILES` is more robust.
    while (!archivedLogs.empty() && archivedLogs.size() >= MAX_LOG_FILES) {
        Serial.printf("[LOGGER] Max log files reached. Deleting oldest archive: %s\n", archivedLogs[0].c_str());
        SdCardManager::deleteFile(archivedLogs[0].c_str());
        archivedLogs.erase(archivedLogs.begin());
    }
}

// The log function implementation is now in the header file because it's a template.
// This .cpp file now only contains setup, manageLogFiles, and the constructor.