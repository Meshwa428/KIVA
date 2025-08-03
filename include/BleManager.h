#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <string>
#include <memory>
#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

class App;
class BleKeyboard;

class BleManager {
public:
    enum class State {
        IDLE,           // Stack is on, but keyboard is not advertising
        KEYBOARD_ACTIVE // Keyboard is advertising and/or connected
    };

    BleManager();
    ~BleManager();

    void setup(App* app);

    // --- Public API for DuckyScriptRunner ---
    void startKeyboard();
    void stopKeyboard();
    BleKeyboard* getKeyboard();
    
    // --- Getters for UI ---
    State getState() const;
    bool isKeyboardConnected() const;

private:
    static void bleTaskWrapper(void* param);
    void taskLoop();

    App* app_;
    TaskHandle_t bleTaskHandle_;
    SemaphoreHandle_t stopSemaphore_; // Used to confirm graceful shutdown

    volatile State currentState_;
    volatile bool startKeyboardRequested_;
    volatile bool stopKeyboardRequested_;
    
    std::unique_ptr<BleKeyboard> bleKeyboard_;
};

#endif // BLE_MANAGER_H