#include "BleManager.h"
#include "App.h"
#include "Logger.h"
#include <NimBLEDevice.h>
#include <BleKeyboard.h>

BleManager::BleManager() :
    app_(nullptr),
    bleTaskHandle_(nullptr),
    stopSemaphore_(nullptr),
    currentState_(State::IDLE),
    startKeyboardRequested_(false),
    stopKeyboardRequested_(false)
{}

BleManager::~BleManager() {
    if (bleTaskHandle_ != nullptr) {
        vTaskDelete(bleTaskHandle_);
    }
    if (stopSemaphore_ != nullptr) {
        vSemaphoreDelete(stopSemaphore_);
    }
}

void BleManager::setup(App* app) {
    app_ = app;
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

// --- Public API ---

void BleManager::startKeyboard() {
    startKeyboardRequested_ = true;
    stopKeyboardRequested_ = false;
}

void BleManager::stopKeyboard() {
    stopKeyboardRequested_ = true;
    startKeyboardRequested_ = false;
    // Block the main thread until the task confirms the keyboard is fully stopped.
    // This prevents race conditions when quickly exiting and re-entering menus.
    xSemaphoreTake(stopSemaphore_, pdMS_TO_TICKS(2000));
}

BleKeyboard* BleManager::getKeyboard() {
    return bleKeyboard_.get();
}

BleManager::State BleManager::getState() const {
    return currentState_;
}

bool BleManager::isKeyboardConnected() const {
    if (bleKeyboard_ && currentState_ == State::KEYBOARD_ACTIVE) {
        return bleKeyboard_->isConnected();
    }
    return false;
}

// --- Task and State Machine ---

void BleManager::bleTaskWrapper(void* param) {
    static_cast<BleManager*>(param)->taskLoop();
}

void BleManager::taskLoop() {
    LOG(LogLevel::INFO, "BLE_MGR", "BLE Manager task started.");
    
    // <-- FIX: Initialize with an empty name to prevent the default "Kiva" device from appearing.
    if(!NimBLEDevice::init("")) {
        LOG(LogLevel::ERROR, "BLE_MGR", "Failed to initialize NimBLE");
        vTaskDelete(nullptr);
        return;
    }
    
    // Create the server and keyboard ONCE. They will live forever.
    NimBLEServer *pServer = NimBLEDevice::createServer();
    bleKeyboard_.reset(new BleKeyboard("Kiva Ducky", "Kiva Systems", 100));

    // The task is now officially idle and ready for commands.
    currentState_ = State::IDLE;

    for (;;) {
        // Check for a request to start the keyboard
        if (startKeyboardRequested_ && currentState_ == State::IDLE) {
            LOG(LogLevel::INFO, "BLE_MGR", "Starting Keyboard Advertising...");
            bleKeyboard_->begin();
            currentState_ = State::KEYBOARD_ACTIVE;
            startKeyboardRequested_ = false;
        }

        // Check for a request to stop the keyboard
        if (stopKeyboardRequested_ && currentState_ == State::KEYBOARD_ACTIVE) {
            LOG(LogLevel::INFO, "BLE_MGR", "Stopping Keyboard...");
            
            if (pServer->getAdvertising()->isAdvertising()) {
                pServer->getAdvertising()->stop();
            }

            if (pServer->getConnectedCount() > 0) {
                LOG(LogLevel::INFO, "BLE_MGR", "Disconnecting %d clients.", pServer->getConnectedCount());
                auto peerHandles = pServer->getPeerDevices();
                for(auto handle : peerHandles) {
                    pServer->disconnect(handle);
                }
                // Give a moment for the OS to process the disconnect cleanly
                vTaskDelay(pdMS_TO_TICKS(200)); 
            }

            // <-- FIX: Call the new end() method to tear down services and the HID device.
            bleKeyboard_->end();

            currentState_ = State::IDLE;
            stopKeyboardRequested_ = false;
            LOG(LogLevel::INFO, "BLE_MGR", "Keyboard stopped. Task is now idle.");

            // Signal the main thread that the stop operation is complete.
            xSemaphoreGive(stopSemaphore_);
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Task loop delay
    }
}