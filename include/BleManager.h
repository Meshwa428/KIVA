#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <string>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

class App;
class BleKeyboard;

class BleManager {
public:
    enum class BleOwner {
        NONE,
        KEYBOARD,
        SPAMMER
    };

    BleManager();
    ~BleManager();

    void setup(App* app);

    bool requestBle(BleOwner owner);
    void releaseBle();
    
    // --- Getters for UI ---
    BleOwner getOwner() const;
    bool isKeyboardConnected() const;
    BleKeyboard* getKeyboard();

private:
    static void bleTaskWrapper(void* param);
    void taskLoop();

    App* app_;
    TaskHandle_t bleTaskHandle_;
    SemaphoreHandle_t requestSemaphore_;
    SemaphoreHandle_t releaseSemaphore_;

    volatile BleOwner currentOwner_;
    volatile BleOwner requestedOwner_;
    
    // unique_ptr to manage the keyboard object's lifetime
    std::unique_ptr<BleKeyboard> bleKeyboard_;
};

#endif // BLE_MANAGER_H