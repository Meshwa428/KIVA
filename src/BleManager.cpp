#include "BleManager.h"
#include "App.h"
#include "Logger.h"
#include <NimBLEDevice.h>
#include <BleKeyboard.h>

BleManager::BleManager() :
    app_(nullptr),
    bleTaskHandle_(nullptr),
    startSemaphore_(nullptr),
    stopSemaphore_(nullptr),
    currentState_(State::IDLE),
    startKeyboardRequested_(false),
    stopKeyboardRequested_(false)
{}

BleManager::~BleManager() {
    if (bleTaskHandle_ != nullptr) vTaskDelete(bleTaskHandle_);
    if (startSemaphore_ != nullptr) vSemaphoreDelete(startSemaphore_);
    if (stopSemaphore_ != nullptr) vSemaphoreDelete(stopSemaphore_);
}

void BleManager::setup(App* app) {
    app_ = app;
    startSemaphore_ = xSemaphoreCreateBinary();
    stopSemaphore_ = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(
        this->bleTaskWrapper,
        "BLEManagerTask",
        4096,
        this,
        5,
        &bleTaskHandle_,
        CONFIG_BT_NIMBLE_PINNED_TO_CORE
    );
}

BleKeyboard* BleManager::startKeyboard() {
    if (currentState_ != State::IDLE) {
        LOG(LogLevel::WARN, "BLE_MGR", "startKeyboard called when not idle. State: %d", (int)currentState_);
        return bleKeyboard_.get();
    }
    
    LOG(LogLevel::INFO, "BLE_MGR", "Main thread requesting keyboard start...");
    startKeyboardRequested_ = true;

    if (xSemaphoreTake(startSemaphore_, pdMS_TO_TICKS(5000)) == pdTRUE) {
        LOG(LogLevel::INFO, "BLE_MGR", "Main thread confirmed keyboard is ready.");
        return bleKeyboard_.get();
    } else {
        LOG(LogLevel::ERROR, "BLE_MGR", "startKeyboard timed out!");
        startKeyboardRequested_ = false;
        return nullptr;
    }
}

void BleManager::stopKeyboard() {
    if (currentState_ != State::KEYBOARD_ACTIVE) {
        return;
    }
    LOG(LogLevel::INFO, "BLE_MGR", "Main thread requesting keyboard stop...");
    stopKeyboardRequested_ = true;

    xSemaphoreTake(stopSemaphore_, pdMS_TO_TICKS(2000));
    LOG(LogLevel::INFO, "BLE_MGR", "Main thread confirmed keyboard is stopped.");
}

bool BleManager::isKeyboardConnected() const {
    if (bleKeyboard_ && currentState_ == State::KEYBOARD_ACTIVE) {
        return bleKeyboard_->isConnected();
    }
    return false;
}

BleManager::State BleManager::getState() const {
    return currentState_;
}


void BleManager::bleTaskWrapper(void* param) {
    static_cast<BleManager*>(param)->taskLoop();
}

void BleManager::taskLoop() {
    LOG(LogLevel::INFO, "BLE_MGR", "BLE Manager task started and is idle.");
    
    NimBLEServer *pServer = nullptr;
    currentState_ = State::IDLE;

    for (;;) {
        if (currentState_ == State::IDLE && startKeyboardRequested_) {
            LOG(LogLevel::INFO, "BLE_MGR_TASK", "STATE_IDLE -> STARTING...");
            
            if(!NimBLEDevice::init("")) {
                LOG(LogLevel::ERROR, "BLE_MGR_TASK", "Failed to initialize NimBLE");
                bleKeyboard_.reset();
            } else {
                pServer = NimBLEDevice::createServer();
                bleKeyboard_.reset(new BleKeyboard("Kiva Ducky", "Kiva Systems", 100));
                bleKeyboard_->begin();
                currentState_ = State::KEYBOARD_ACTIVE;
            }
            
            startKeyboardRequested_ = false;
            xSemaphoreGive(startSemaphore_);

        } else if (currentState_ == State::KEYBOARD_ACTIVE && stopKeyboardRequested_) {
            LOG(LogLevel::INFO, "BLE_MGR_TASK", "STATE_ACTIVE -> STOPPING...");

            // --- THIS IS THE CORRECTED, SAFE SHUTDOWN SEQUENCE ---
            // 1. Stop advertising and disconnect clients (part of NimBLE state)
            if (pServer && pServer->getAdvertising()->isAdvertising()) {
                pServer->getAdvertising()->stop();
            }
            if (pServer && pServer->getConnectedCount() > 0) {
                 auto peerDevices = pServer->getPeerDevices();
                 for(auto& peer : peerDevices) pServer->disconnect(peer);
                 vTaskDelay(pdMS_TO_TICKS(200));
            }

            // 2. Clean up services and HID device *within* the keyboard object.
            // The BleKeyboard object itself still exists.
            if(bleKeyboard_) {
                bleKeyboard_->end();
            }
            
            // 3. De-initialize the entire NimBLE stack. This correctly cleans up
            // the server and removes its internal pointer to our BleKeyboard object.
            if(NimBLEDevice::isInitialized()) {
                NimBLEDevice::deinit(true);
            }
            
            // 4. NOW it is safe to destroy the BleKeyboard C++ object.
            bleKeyboard_.reset();
            
            pServer = nullptr;
            currentState_ = State::IDLE;
            stopKeyboardRequested_ = false;
            LOG(LogLevel::INFO, "BLE_MGR_TASK", "Keyboard stopped & stack de-initialized.");
            xSemaphoreGive(stopSemaphore_); // Unblock main thread
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}