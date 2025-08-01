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
class BleManagerServerCallbacks; // Forward declare

class BleManager {
public:
    enum class State {
        OFF,
        IDLE,
        STARTING_KEYBOARD,
        KEYBOARD_ACTIVE,
        STOPPING_KEYBOARD
    };

    BleManager();
    ~BleManager();

    void setup(App* app);

    bool requestKeyboard();
    void releaseKeyboard();
    BleKeyboard* getKeyboard();
    State getState() const;
    
    // Public method for the callback to access the semaphore
    void signalDisconnect();

private:
    static void bleTaskWrapper(void* param);
    void taskLoop();

    void handleStateIdle();
    void handleStateStartingKeyboard();
    void handleStateKeyboardActive();
    void handleStateStoppingKeyboard();

    App* app_;
    TaskHandle_t bleTaskHandle_;
    SemaphoreHandle_t syncSemaphore_;

    volatile State currentState_;
    volatile State requestedState_;
    
    std::unique_ptr<BleKeyboard> bleKeyboard_;
    
    // --- The Fix: Callback handler is now a member ---
    std::unique_ptr<BleManagerServerCallbacks> serverCallbacks_; 
};

#endif // BLE_MANAGER_H