#include "Logger.h"
#include "SdCardManager.h"
#include <vector>
#include <algorithm>

// Singleton instance definition
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : logDir_(SD_ROOT::DATA_LOGS), logBaseName_(SD_ROOT::LOG_BASE_NAME), logExtension_(".txt") {}

void Logger::setup() {
    if (!SdCardManager::isAvailable()) {
        Serial.println("[LOGGER] SD Card not available. File logging disabled.");
        isInitialized_ = false;
        return;
    }

    // Ensure the logs directory exists
    if (!SdCardManager::exists(logDir_)) {
        SdCardManager::createDir(logDir_);
    }

    manageLogFiles();

    // Create the new log file path
    char newLogFileName[48];
    snprintf(newLogFileName, sizeof(newLogFileName), "%s%s%lu%s", logDir_, logBaseName_, millis(), logExtension_);
    currentLogFile_ = newLogFileName;

    isInitialized_ = true;
    Serial.printf("[LOGGER] Logging to new file: %s\n", currentLogFile_.c_str());
    log(LogLevel::INFO, "LOGGER", "--- System Boot ---");
}

void Logger::manageLogFiles() {
    std::vector<String> logFiles;
    File root = SdCardManager::openFile(logDir_);
    if (!root || !root.isDirectory()) {
        return;
    }

    File file = root.openNextFile();
    while(file) {
        String fileName = file.name();
        if (!file.isDirectory() && fileName.startsWith(logBaseName_) && fileName.endsWith(logExtension_)) {
            logFiles.push_back(String(logDir_) + "/" + fileName);
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();

    // Sort files alphabetically (which also sorts by timestamp in the name)
    std::sort(logFiles.begin(), logFiles.end());

    // If we have too many log files, delete the oldest ones
    while (logFiles.size() >= MAX_LOG_FILES) {
        Serial.printf("[LOGGER] Deleting old log file: %s\n", logFiles[0].c_str());
        SdCardManager::deleteFile(logFiles[0].c_str());
        logFiles.erase(logFiles.begin());
    }
}

void Logger::log(LogLevel level, const char* component, const char* format, ...) {
    char buffer[256];
    char levelChar;

    switch(level) {
        case LogLevel::DEBUG: levelChar = 'D'; break;
        case LogLevel::INFO:  levelChar = 'I'; break;
        case LogLevel::WARN:  levelChar = 'W'; break;
        case LogLevel::ERROR: levelChar = 'E'; break;
        default:              levelChar = '?'; break;
    }

    // Format the log message with timestamp, level, component, etc.
    int prefixLen = snprintf(buffer, sizeof(buffer), "%lu [%c] [%s] ", millis(), levelChar, component);

    // Format the user's message
    va_list args;
    va_start(args, format);
    vsnprintf(buffer + prefixLen, sizeof(buffer) - prefixLen, format, args);
    va_end(args);

    // Print to Serial monitor
    Serial.println(buffer);

    // Print to SD card file
    if (isInitialized_) {
        File logFile = SdCardManager::openFile(currentLogFile_.c_str(), FILE_APPEND);
        if (logFile) {
            logFile.println(buffer);
            logFile.close();
        }
    }
}