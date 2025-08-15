#include "BleManager.h"
#include "App.h"
#include "Logger.h"
#include <NimBLEDevice.h>
#include <BleKeyboard.h>

BleManager::BleManager() :
    app_(nullptr),
    bleTaskHandle_(nullptr),
    requestSemaphore_(nullptr),
    releaseSemaphore_(nullptr),
    currentOwner_(BleOwner::NONE),
    requestedOwner_(BleOwner::NONE)
{}

BleManager::~BleManager() {
    if (bleTaskHandle_ != nullptr) vTaskDelete(bleTaskHandle_);
    if (requestSemaphore_ != nullptr) vSemaphoreDelete(requestSemaphore_);
    if (releaseSemaphore_ != nullptr) vSemaphoreDelete(releaseSemaphore_);
}

void BleManager::setup(App* app) {
    app_ = app;
    requestSemaphore_ = xSemaphoreCreateBinary();
    releaseSemaphore_ = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(
        this->bleTaskWrapper,
        "BLEManagerTask",
        4096, // Increased stack size for BLE
        this,
        5, // Task priority
        &bleTaskHandle_,
        CONFIG_BT_NIMBLE_PINNED_TO_CORE
    );
}

bool BleManager::requestBle(BleOwner owner) {
    if (currentOwner_ != BleOwner::NONE) {
        LOG(LogLevel::WARN, "BLE_MGR", "requestBle called when an owner already exists. Call releaseBle first.");
        return false;
    }
    if (owner == BleOwner::NONE) {
        LOG(LogLevel::WARN, "BLE_MGR", "Cannot request owner NONE.");
        return false;
    }

    LOG(LogLevel::INFO, "BLE_MGR", "Main thread requesting BLE for owner: %d", (int)owner);
    requestedOwner_ = owner;

    // Wait for the BLE task to confirm startup
    if (xSemaphoreTake(requestSemaphore_, pdMS_TO_TICKS(10000)) == pdTRUE) {
        LOG(LogLevel::INFO, "BLE_MGR", "Main thread confirmed BLE is ready for owner: %d", (int)owner);
        return true;
    } else {
        LOG(LogLevel::ERROR, "BLE_MGR", "requestBle timed out waiting for owner: %d", (int)owner);
        requestedOwner_ = BleOwner::NONE; // Cancel request
        return false;
    }
}

void BleManager::releaseBle() {
    if (currentOwner_ == BleOwner::NONE) {
        return;
    }
    LOG(LogLevel::INFO, "BLE_MGR", "Main thread requesting BLE release from owner: %d", (int)currentOwner_);

    // Request the task to stop the current owner
    requestedOwner_ = BleOwner::NONE;

    // Wait for the BLE task to confirm shutdown
    if (xSemaphoreTake(releaseSemaphore_, pdMS_TO_TICKS(5000)) == pdTRUE) {
        LOG(LogLevel::INFO, "BLE_MGR", "Main thread confirmed BLE is released.");
    } else {
        LOG(LogLevel::ERROR, "BLE_MGR", "releaseBle timed out.");
    }
}

BleManager::BleOwner BleManager::getOwner() const {
    return currentOwner_;
}

bool BleManager::isKeyboardConnected() const {
    if (bleKeyboard_ && currentOwner_ == BleOwner::KEYBOARD) {
        return bleKeyboard_->isConnected();
    }
    return false;
}

BleKeyboard* BleManager::getKeyboard() {
    return bleKeyboard_.get();
}


// --- Task ---

void BleManager::bleTaskWrapper(void* param) {
    static_cast<BleManager*>(param)->taskLoop();
}

void BleManager::taskLoop() {
    LOG(LogLevel::INFO, "BLE_MGR", "BLE Manager task started.");
    
    for (;;) {
        // State transition logic
        if (requestedOwner_ != currentOwner_) {
            
            // --- 1. TEARDOWN CURRENT OWNER (if any) ---
            if (currentOwner_ != BleOwner::NONE) {
                LOG(LogLevel::INFO, "BLE_MGR_TASK", "Tearing down owner: %d", (int)currentOwner_);

                if (currentOwner_ == BleOwner::KEYBOARD) {
                    if (bleKeyboard_) {
                        // Safe shutdown for keyboard
                        if (NimBLEDevice::getServer()->getAdvertising()->isAdvertising()) {
                             NimBLEDevice::getServer()->getAdvertising()->stop();
                        }
                        if (NimBLEDevice::getServer()->getConnectedCount() > 0) {
                            NimBLEDevice::getServer()->disconnect(NimBLEDevice::getServer()->getPeerDevices()[0]);
                             vTaskDelay(pdMS_TO_TICKS(100));
                        }
                        bleKeyboard_->end();
                    }
                }
                // (Future: Add SPAMMER teardown if needed)

                // De-initialize the entire NimBLE stack. A call to deinit() is safe
                // even if the stack is not currently initialized.
                NimBLEDevice::deinit();
                LOG(LogLevel::INFO, "BLE_MGR_TASK", "NimBLE stack de-initialized.");

                bleKeyboard_.reset(); // Destroy the C++ object

                LOG(LogLevel::INFO, "BLE_MGR_TASK", "Owner %d teardown complete.", (int)currentOwner_);

                // If the request was to stop (i.e., release), unblock the caller.
                if (requestedOwner_ == BleOwner::NONE) {
                    xSemaphoreGive(releaseSemaphore_);
                }
                currentOwner_ = BleOwner::NONE;
            }

            // --- 2. SETUP NEW OWNER (if any) ---
            if (requestedOwner_ != BleOwner::NONE) {
                LOG(LogLevel::INFO, "BLE_MGR_TASK", "Setting up new owner: %d", (int)requestedOwner_);

                // Initialize BLE stack
                NimBLEDevice::init("");

                if (requestedOwner_ == BleOwner::KEYBOARD) {
                    NimBLEDevice::createServer();
                    bleKeyboard_.reset(new BleKeyboard("Kiva Ducky", "Kiva Systems", 100));
                    bleKeyboard_->begin();
                } else if (requestedOwner_ == BleOwner::SPAMMER) {
                    // For the spammer, we just need the stack to be up.
                    // The BleSpammer class will handle advertising directly.
                }

                currentOwner_ = requestedOwner_;
                LOG(LogLevel::INFO, "BLE_MGR_TASK", "Owner %d setup complete.", (int)currentOwner_);
                xSemaphoreGive(requestSemaphore_); // Unblock the caller
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}