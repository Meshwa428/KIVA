#ifndef DUCKY_SCRIPT_RUNNER_H
#define DUCKY_SCRIPT_RUNNER_H

#include "SdCardManager.h"
#include <string>
#include <memory>
#include <vector>
#include <HIDForge.h> // <-- ADD THIS

// Forward Declarations
class App;

// --- Ducky Script Keywords ---
struct DuckyCommand {
    const char* command;
    uint8_t key;
    enum class Type { UNKNOWN, CMD, PRINT, DELAY, REPEAT, COMBINATION } type;
};

struct DuckyCombination {
    const char* command;
    uint8_t key1;
    uint8_t key2;
    uint8_t key3;
};

#include "Service.h"

class DuckyScriptRunner : public Service {
public:
    enum class Mode { USB, BLE };
    enum class State { IDLE, WAITING_FOR_CONNECTION, POST_CONNECTION_DELAY, RUNNING, FINISHED };

    DuckyScriptRunner();
    void setup(App* app) override;
    void loop();

    bool startScript(const std::string& scriptPath, Mode mode);
    void stopScript();

    State getState() const;
    Mode getMode() const; // <-- ADD THIS
    bool isActive() const;
    const std::string& getCurrentLine() const;
    const std::string& getScriptName() const;
    uint32_t getLinesExecuted() const;
    
private:
    void parseAndExecute(const std::string& line);
    void executeCombination(const std::string& command, const std::string& arg);

    App* app_;
    SdCardManager::LineReader scriptReader_;
    
    std::unique_ptr<HIDInterface> activeHid_;
    
    State state_;
    Mode currentMode_;
    std::string scriptName_;
    std::string currentLine_;
    uint32_t linesExecuted_;
    
    unsigned long delayUntil_;
    int defaultDelay_;
    unsigned long connectionTime_;
    
    std::string lastLine_;
};

#endif // DUCKY_SCRIPT_RUNNER_H