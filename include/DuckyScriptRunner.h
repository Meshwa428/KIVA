#ifndef DUCKY_SCRIPT_RUNNER_H
#define DUCKY_SCRIPT_RUNNER_H

#include "SdCardManager.h"
#include <HIDForge.h>
#include <string>

class App;

class DuckyScriptRunner {
public:
    enum class State { IDLE, WAITING_FOR_CONNECTION, RUNNING, FINISHED };

    DuckyScriptRunner();
    void setup(App* app);
    void loop();

    // The signature now accepts an interface pointer and a boolean type.
    bool startScript(const std::string& scriptPath, HIDInterface* hid, bool isUsb);
    void stopScript();

    State getState() const;
    bool isActive() const;
    const std::string& getScriptName() const;
    uint32_t getLinesExecuted() const;
    HIDInterface* getHidInterface() { return activeHid_; }

private:
    void parseAndExecute(const std::string& line);

    App* app_;
    SdCardManager::LineReader scriptReader_;
    
    HIDInterface* activeHid_; // Pointer to the active HID object (owned by App)
    bool isUsb_;              // This flag replaces dynamic_cast
    
    State state_;
    std::string scriptName_;
    uint32_t linesExecuted_;
    
    unsigned long delayUntil_;
    int defaultDelay_;
    unsigned long connectionTime_;
    
    std::string lastLine_;
};

#endif // DUCKY_SCRIPT_RUNNER_H