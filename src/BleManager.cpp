#include "BleManager.h"
#include "App.h"
#include "Logger.h"
#include <NimBLEDevice.h>
#include <BleKeyboard.h>

// --- Helper Class for Server Callbacks ---
class BleManagerServerCallbacks : public NimBLEServerCallbacks {
public:
    BleManagerServerCallbacks(SemaphoreHandle_t semaphore) : disconnectSemaphore(semaphore) {}

    void onDisconnect(NimBLEServer* pServer) override {
        LOG(LogLevel::INFO, "BLE_MGR_CB", "Client disconnected.");
        // Give the semaphore to signal that the disconnect is complete.
        xSemaphoreGive(disconnectSemaphore);
    }

private:
    SemaphoreHandle_t disconnectSemaphore;
};

BleManager::BleManager() :
    app_(nullptr),
    bleTaskHandle_(nullptr),
    syncSemaphore_(nullptr),
    currentState_(State::OFF),
    requestedState_(State::OFF)
{}

BleManager::~BleManager() {
    if (bleTaskHandle_ != nullptr) {
        vTaskDelete(bleTaskHandle_);
    }
    if (syncSemaphore_ != nullptr) {
        vSemaphoreDelete(syncSemaphore_);
    }
}

void BleManager::setup(App* app) {
    app_ = app;
    syncSemaphore_ = xSemaphoreCreateBinary();

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

bool BleManager::requestKeyboard() {
    if (currentState_ != State::IDLE) {
        LOG(LogLevel::WARN, "BLE_MGR", "Request for keyboard while not idle.");
        return false;
    }
    LOG(LogLevel::INFO, "BLE_MGR", "Keyboard requested. Waking task and waiting for completion.");
    requestedState_ = State::STARTING_KEYBOARD;

    if (xSemaphoreTake(syncSemaphore_, pdMS_TO_TICKS(2000)) == pdTRUE) {
        LOG(LogLevel::INFO, "BLE_MGR", "Semaphore received. Keyboard is ready.");
        return true;
    } else {
        LOG(LogLevel::ERROR, "BLE_MGR", "Timed out waiting for keyboard semaphore.");
        requestedState_ = State::STOPPING_KEYBOARD;
        return false;
    }
}

void BleManager::releaseKeyboard() {
    if (currentState_ == State::KEYBOARD_ACTIVE || currentState_ == State::STARTING_KEYBOARD) {
        LOG(LogLevel::INFO, "BLE_MGR", "Keyboard released. Idling task.");
        requestedState_ = State::STOPPING_KEYBOARD;
        xSemaphoreTake(syncSemaphore_, pdMS_TO_TICKS(2000)); // Wait for full shutdown
    }
}

BleKeyboard* BleManager::getKeyboard() {
    return bleKeyboard_.get();
}

BleManager::State BleManager::getState() const {
    return currentState_;
}

void BleManager::bleTaskWrapper(void* param) {
    static_cast<BleManager*>(param)->taskLoop();
}

void BleManager::taskLoop() {
    LOG(LogLevel::INFO, "BLE_MGR", "BLE Manager task started.");
    
    NimBLEDevice::init("Kiva");
    
    // --- Set up the server and its callback handler ONCE ---
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new BleManagerServerCallbacks(syncSemaphore_));

    currentState_ = State::IDLE;
    requestedState_ = State::IDLE;

    for (;;) {
        if (requestedState_ != currentState_) {
            currentState_ = requestedState_;
        }

        switch (currentState_) {
            case State::IDLE:               handleStateIdle();              break;
            case State::STARTING_KEYBOARD:  handleStateStartingKeyboard();  break;
            case State::KEYBOARD_ACTIVE:    handleStateKeyboardActive();    break;
            case State::STOPPING_KEYBOARD:  handleStateStoppingKeyboard();  break;
            default: break;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void BleManager::handleStateIdle() {
    // Task waits for a request.
}

void BleManager::handleStateStartingKeyboard() {
    LOG(LogLevel::INFO, "BLE_MGR", "State: STARTING KEYBOARD");
    bleKeyboard_.reset(new BleKeyboard("Kiva Ducky", "Kiva Systems", 100));
    bleKeyboard_->begin();
    
    requestedState_ = State::KEYBOARD_ACTIVE;
    xSemaphoreGive(syncSemaphore_);
}

void BleManager::handleStateKeyboardActive() {
    // Task waits for a stop command.
}

void BleManager::handleStateStoppingKeyboard() {
    LOG(LogLevel::INFO, "BLE_MGR", "State: STOPPING KEYBOARD");
    NimBLEServer *pServer = NimBLEDevice::getServer();

    // --- NEW GRACEFUL SHUTDOWN LOGIC ---
    if (pServer && pServer->getConnectedCount() > 0) {
        LOG(LogLevel::INFO, "BLE_MGR", "Client is connected. Requesting disconnect.");
        pServer->disconnect(pServer->getConnectedCount());
        // Now we wait. The onDisconnect callback will give the semaphore.
        if (xSemaphoreTake(syncSemaphore_, pdMS_TO_TICKS(1500)) != pdTRUE) {
            LOG(LogLevel::WARN, "BLE_MGR", "Timed out waiting for client to disconnect.");
        }
    }

    if (bleKeyboard_) {
        bleKeyboard_->end();
        bleKeyboard_.reset();
    }
    
    requestedState_ = State::IDLE;
    LOG(LogLevel::INFO, "BLE_MGR", "BLE keyboard stopped. Returning to IDLE.");
    
    // Signal the main thread that the full shutdown is complete.
    xSemaphoreGive(syncSemaphore_);
}