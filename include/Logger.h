#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "Config.h"
#include <cstdarg>
#include "SdCardManager.h" // <-- ADD THIS LINE

// Define a log level for filtering
enum class LogLevel {
    DEBUG,   // Verbose, for development
    INFO,    // Standard operational messages
    WARN,    // Potential issues
    ERROR    // Critical errors
};

class Logger {
public:
    static Logger& getInstance();
    void setup();

    // --- TEMPLATE-BASED SOLUTION ---

    // Base implementation with the file flag
    template<typename... Args>
    void log(LogLevel level, const char* component, bool toFile, const char* format, Args... args) {
        char buffer[256];
        char levelChar;

        switch(level) {
            case LogLevel::DEBUG: levelChar = 'D'; break;
            case LogLevel::INFO:  levelChar = 'I'; break;
            case LogLevel::WARN:  levelChar = 'W'; break;
            case LogLevel::ERROR: levelChar = 'E'; break;
            default:              levelChar = '?'; break;
        }

        int prefixLen = snprintf(buffer, sizeof(buffer), "%lu [%c] [%s] ", millis(), levelChar, component);
        if (prefixLen >= 0 && prefixLen < sizeof(buffer)) {
            snprintf(buffer + prefixLen, sizeof(buffer) - prefixLen, format, args...);
        }

        Serial.println(buffer);

        if (toFile && isInitialized_ && !currentLogFile_.isEmpty()) {
            File logFile = SdCardManager::openFile(currentLogFile_.c_str(), FILE_APPEND);
            if (logFile) {
                logFile.println(buffer);
                logFile.close();
            }
        }
    }

    // Overload that defaults to file logging.
    // This is a better match for calls where the third argument is a string literal.
    template<typename... Args>
    void log(LogLevel level, const char* component, const char* format, Args... args) {
        // Call the main implementation with toFile = true
        log(level, component, true, format, args...);
    }
    
    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;

private:
    Logger();
    void manageLogFiles();

    static const int MAX_LOG_FILES = 3;
    bool isInitialized_ = false;
    String currentLogFile_;
};

// This macro now works perfectly with the template overloads.
#define LOG(level, component, ...) Logger::getInstance().log(level, component, ##__VA_ARGS__)

#endif // LOGGER_H