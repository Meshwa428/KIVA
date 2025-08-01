#ifndef DUCKY_SCRIPT_RUNNER_H
#define DUCKY_SCRIPT_RUNNER_H

#include "HardwareManager.h"
#include "SdCardManager.h"
#include <string>
#include <memory>
#include <vector>

// Forward Declarations
class USBHIDKeyboard;
class BleKeyboard;
class App;

// --- HID Interface (Restored) ---
class HIDInterface {
public:
    enum class Type { USB, BLE };
    virtual ~HIDInterface() {}
    virtual bool begin() = 0;
    virtual void end() = 0;
    virtual size_t press(uint8_t k) = 0;
    virtual size_t release(uint8_t k) = 0;
    virtual void releaseAll() = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) = 0;
    virtual bool isConnected() = 0;
    virtual Type getType() const = 0;
};

// --- Concrete BLE HID Implementation ---
class BleHid : public HIDInterface {
public:
    BleHid(BleKeyboard* keyboard); // Constructor takes a pointer
    ~BleHid();
    bool begin() override;
    void end() override;
    size_t press(uint8_t k) override;
    size_t release(uint8_t k) override;
    void releaseAll() override;
    size_t write(const uint8_t *buffer, size_t size) override;
    bool isConnected() override;
    Type getType() const override { return Type::BLE; }
private:
    BleKeyboard* bleKeyboard_; // Stores a pointer, doesn't own it
};

// --- Concrete USB HID Implementation ---
class UsbHid : public HIDInterface {
public:
    UsbHid();
    ~UsbHid();
    bool begin() override;
    void end() override;
    size_t press(uint8_t k) override;
    size_t release(uint8_t k) override;
    void releaseAll() override;
    size_t write(const uint8_t *buffer, size_t size) override;
    bool isConnected() override;
    Type getType() const override { return Type::USB; }
private:
    std::unique_ptr<USBHIDKeyboard> usb_keyboard_;
};

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

class DuckyScriptRunner {
public:
    enum class Mode { USB, BLE };
    enum class State { IDLE, WAITING_FOR_CONNECTION, RUNNING, FINISHED };

    DuckyScriptRunner();
    void setup(App* app);
    void loop();

    // --- Back to original, powerful startScript ---
    bool startScript(const std::string& scriptPath, Mode mode);
    void stopScript();

    State getState() const;
    bool isActive() const;
    const std::string& getCurrentLine() const;
    const std::string& getScriptName() const;
    uint32_t getLinesExecuted() const;
    
private:
    void parseAndExecute(const std::string& line);
    void executeCombination(const std::string& command, const std::string& arg);

    App* app_;
    SdCardManager::LineReader scriptReader_;
    
    std::unique_ptr<HIDInterface> activeHid_; // Manages the active interface
    
    State state_;
    Mode currentMode_;
    std::string scriptName_;
    std::string currentLine_;
    uint32_t linesExecuted_;
    
    unsigned long delayUntil_;
    int defaultDelay_;
    
    std::string lastLine_;
};

#endif // DUCKY_SCRIPT_RUNNER_H