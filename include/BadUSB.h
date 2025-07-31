#ifndef BAD_USB_H
#define BAD_USB_H

#include "HardwareManager.h"
#include "SdCardManager.h"
#include <string>
#include <memory>
#include <vector>

// --- MODIFICATION: REVERT to Forward Declarations ---
// This prevents header conflicts.
class USBHIDKeyboard;
class BleKeyboard;
// --- END MODIFICATION ---

// Forward Declarations to resolve header conflicts
class App;

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
    BleHid(const std::string& deviceName = "Kiva BadUSB");
    ~BleHid(); // <--- We will define the destructor in the .cpp file
    bool begin() override;
    void end() override;
    size_t press(uint8_t k) override;
    size_t release(uint8_t k) override;
    void releaseAll() override;
    size_t write(const uint8_t *buffer, size_t size) override;
    bool isConnected() override;
    Type getType() const override { return Type::BLE; }

private:
    std::unique_ptr<BleKeyboard> bleKeyboard_;
};

// --- Concrete USB HID Implementation ---
class UsbHid : public HIDInterface {
public:
    UsbHid();
    ~UsbHid(); // <--- We will define the destructor in the .cpp file
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

// --- Main BadUSB Manager Class ---
class BadUSB {
public:
    enum class Mode { USB, BLE };
    enum class State { IDLE, WAITING_FOR_CONNECTION, RUNNING, FINISHED };

    BadUSB();
    void setup(App* app);
    void loop();

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
    
    HIDInterface* activeHid_;
    BleHid bleHid_;
    UsbHid usbHid_;
    
    State state_;
    Mode currentMode_;
    std::string scriptName_;
    std::string currentLine_;
    uint32_t linesExecuted_;
    
    unsigned long delayUntil_;
    int defaultDelay_;
    
    std::string lastLine_;
};

#endif // BAD_USB_H