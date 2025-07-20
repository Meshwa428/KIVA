#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <cstdarg> // For va_list, va_start, va_end

// Define a log level for filtering
enum class LogLevel {
    DEBUG,   // Verbose, for development
    INFO,    // Standard operational messages
    WARN,    // Potential issues
    ERROR    // Critical errors
};

class Logger {
public:
    // Singleton access method
    static Logger& getInstance();

    // Setup function to be called once at boot
    void setup();

    // The main logging function
    void log(LogLevel level, const char* component, const char* format, ...);

    // Disable copy and assignment
    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;

private:
    Logger(); // Private constructor
    void manageLogFiles();

    static const int MAX_LOG_FILES = 3;
    const char* logDir_ = "/logs";
    const char* logBaseName_ = "/log_";
    const char* logExtension_ = ".txt";

    bool isInitialized_ = false;
    String currentLogFile_;
};

// A simple macro to make logging easier
#define LOG(level, component, format, ...) Logger::getInstance().log(level, component, format, ##__VA_ARGS__)

#endif // LOGGER_H