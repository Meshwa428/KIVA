#ifndef DUCKY_SCRIPT_RUNNER_H
#define DUCKY_SCRIPT_RUNNER_H

#include "SdCardManager.h"
#include <HIDForge.h> // Use the new library
#include <string>
#include <memory>
#include <vector>

class App;

class DuckyScriptRunner {
public:
    enum class State { IDLE, WAITING_FOR_CONNECTION, RUNNING, FINISHED };

    DuckyScriptRunner();
    void setup(App* app);
    void loop();

    bool startScript(const std::string& scriptPath, HIDInterface* hid);
    void stopScript();

    State getState() const;
    bool isActive() const;
    const std::string& getScriptName() const;
    uint32_t getLinesExecuted() const;
    
private:
    void parseAndExecute(const std::string& line);

    App* app_;
    SdCardManager::LineReader scriptReader_;
    
    HIDInterface* activeHid_; // Raw pointer, lifetime managed by App
    
    State state_;
    std::string scriptName_;
    uint32_t linesExecuted_;
    
    unsigned long delayUntil_;
    int defaultDelay_;
    unsigned long connectionTime_;
    
    std::string lastLine_;
};

#endif // DUCKY_SCRIPT_RUNNER_H