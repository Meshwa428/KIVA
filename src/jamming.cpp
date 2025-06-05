#include "jamming.h"
#include "esp_bt.h"     // For esp_bt_controller_deinit/init
#include "esp_wifi.h"   // For esp_wifi_stop/deinit/init/start
#include <SPI.h>        // For SPI communication with NRF24

// NRF24L01 radio objects
// Pins should be defined in config.h and passed here or used directly
RF24 radio1(NRF1_CE_PIN, NRF1_CSN_PIN, SPI_SPEED_NRF);
RF24 radio2(NRF2_CE_PIN, NRF2_CSN_PIN, SPI_SPEED_NRF);

// Channel lists
const int ble_adv_nrf_channels[] = {2, 26, 80}; 
const int bt_classic_nrf_channels[] = {
    2,  5,  8, 11, 14, 17, 20, 23, 26, 29,
    32, 35, 38, 41, 44, 47, 50, 53, 56, 59,
    62, 65, 68, 71, 74, 77, 80             
}; 

static int current_ble_channel_idx = 0;
static int current_bt_channel_idx = 0;
static int current_nrf_channel_sweep = 0;

bool configureRadioForJamming(RF24 &radio, const char* radioName) {
    Serial.printf("Jamming: Initializing %s... ", radioName);
    if (radio.begin(&SPI)) { // Pass SPIClass instance if not default
        radio.powerUp(); // Explicitly power up the radio module
        delayMicroseconds(5000); // Allow radio to stabilize after powerUp

        radio.setAutoAck(false);
        radio.stopListening(); // Ensure it's in TX mode contextually
        radio.setRetries(0, 0); // No retransmissions
        radio.setPALevel(RF24_PA_MAX, true); // Max power
        radio.setDataRate(RF24_2MBPS); // Higher data rate can mean wider signal
        radio.setCRCLength(RF24_CRC_DISABLED); // No CRC needed for jamming
        Serial.println("OK.");
        return true;
    }
    Serial.println("FAILED.");
    return false;
}

void setupJamming() {
    Serial.println("Jamming: System setup complete (NRF init deferred to startActiveJamming).");
}

bool startActiveJamming(JammingType type) {
    if (isJammingOperationActive) {
        Serial.println("Jamming: Already active. Stopping current then starting new.");
        stopActiveJamming(); 
        delay(100); 
    }

    Serial.printf("Jamming: Attempting to start mode %d.\n", type);
    activeJammingType = type;

    Serial.println("Jamming: De-initializing ESP32 BT/Wi-Fi controllers...");
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_bt_controller_deinit();
    Serial.println("Jamming: ESP32 radios de-initialized.");

    bool radio1_ok = configureRadioForJamming(radio1, "NRF24L01 #1");
    bool radio2_ok = configureRadioForJamming(radio2, "NRF24L01 #2");

    if (!radio1_ok && !radio2_ok) {
        Serial.println("Jamming: CRITICAL - Both NRF modules failed to initialize.");
        activeJammingType = JAM_NONE;
        isJammingOperationActive = false;
        // It's important to re-initialize ESP32 radios if jamming failed to start
        // to allow normal Wi-Fi/BT functionality.
        // However, the current Kiva flow requires manual Wi-Fi re-enablement,
        // so we'll stick to that for consistency.
        Serial.println("Jamming: ESP32 BT/Wi-Fi controllers remain de-initialized. Re-enable manually if needed.");
        return false;
    }
    if (!radio1_ok) Serial.println("Jamming: Warning - NRF24L01 #1 failed, proceeding with #2 only.");
    if (!radio2_ok) Serial.println("Jamming: Warning - NRF24L01 #2 failed, proceeding with #1 only.");

    int initialChannel = 0;
    switch (type) {
        case JAM_BLE:
            initialChannel = ble_adv_nrf_channels[0];
            current_ble_channel_idx = 0;
            break;
        case JAM_BT:
            initialChannel = bt_classic_nrf_channels[0];
            current_bt_channel_idx = 0;
            break;
        case JAM_NRF_CHANNELS:
        case JAM_RF_NOISE_FLOOD:
            initialChannel = 0;
            current_nrf_channel_sweep = 0;
            break;
        default:
            Serial.println("Jamming: Invalid type for start.");
            activeJammingType = JAM_NONE;
            // Re-initialize ESP32 radios as above if start fails.
            Serial.println("Jamming: ESP32 BT/Wi-Fi controllers remain de-initialized. Re-enable manually if needed.");
            return false;
    }

    if (radio1_ok && radio1.isChipConnected()) radio1.startConstCarrier(RF24_PA_MAX, initialChannel); // Use RF24_PA_MAX
    if (radio2_ok && radio2.isChipConnected()) radio2.startConstCarrier(RF24_PA_MAX, initialChannel); // Use RF24_PA_MAX
    
    isJammingOperationActive = true;
    Serial.printf("Jamming: Mode %d started. Initial channel: %d\n", type, initialChannel);
    return true;
}

void stopActiveJamming() {
    Serial.println("Jamming: Stopping active jamming...");
    if (radio1.isChipConnected()) {
        radio1.stopConstCarrier(); // Stop RF transmission
        radio1.powerDown();        // Put NRF1 into low power mode
        Serial.println("Jamming: NRF1 stopped and powered down.");
    } else {
        Serial.println("Jamming: NRF1 was not connected or already stopped.");
    }

    if (radio2.isChipConnected()) {
        radio2.stopConstCarrier(); // Stop RF transmission
        radio2.powerDown();        // Put NRF2 into low power mode
        Serial.println("Jamming: NRF2 stopped and powered down.");
    } else {
        Serial.println("Jamming: NRF2 was not connected or already stopped.");
    }
    
    activeJammingType = JAM_NONE;
    isJammingOperationActive = false;

    // ESP32's own Wi-Fi/BT controllers were de-initialized when jamming started.
    // They remain de-initialized. To use Wi-Fi or Bluetooth again, Kiva's
    // Wi-Fi setup menu (which calls setWifiHardwareState(true)) must be used.
    // setWifiHardwareState(true) will perform the necessary esp_wifi_init/start.
    // Similar logic would be needed for Bluetooth if it were to be re-enabled programmatically.
    Serial.println("Jamming: ESP32 BT/Wi-Fi controllers remain de-initialized.");
    Serial.println("Jamming: To use Wi-Fi, re-enable via Kiva settings.");
    Serial.println("Jamming: System returned to normal operation mode.");
}

void runJammerCycle() {
    if (!isJammingOperationActive) return;

    int nextChannel1 = -1, nextChannel2 = -1; 

    switch (activeJammingType) {
        case JAM_BLE:
            current_ble_channel_idx = (current_ble_channel_idx + 1) % (sizeof(ble_adv_nrf_channels) / sizeof(ble_adv_nrf_channels[0]));
            nextChannel1 = ble_adv_nrf_channels[current_ble_channel_idx];
            nextChannel2 = ble_adv_nrf_channels[current_ble_channel_idx];
            break;

        case JAM_BT:
            current_bt_channel_idx = (current_bt_channel_idx + 1) % (sizeof(bt_classic_nrf_channels) / sizeof(bt_classic_nrf_channels[0]));
            nextChannel1 = bt_classic_nrf_channels[current_bt_channel_idx];
            nextChannel2 = bt_classic_nrf_channels[current_bt_channel_idx]; 
            break;

        case JAM_NRF_CHANNELS: 
            current_nrf_channel_sweep = (current_nrf_channel_sweep + 1) % 126; 
            nextChannel1 = current_nrf_channel_sweep;
            nextChannel2 = current_nrf_channel_sweep; 
            break;

        case JAM_RF_NOISE_FLOOD: 
            nextChannel1 = random(0, 126); 
            nextChannel2 = random(0, 126); 
            break;

        default:
            return; 
    }

    if (nextChannel1 != -1 && radio1.isChipConnected()) {
        // The RF24 library requires re-calling startConstCarrier to change channel while in CW mode.
        // Some libraries might allow setChannel then a register write, but startConstCarrier is safer.
        radio1.startConstCarrier(RF24_PA_MAX, nextChannel1); // Use RF24_PA_MAX
    }
    if (nextChannel2 != -1 && radio2.isChipConnected()) {
        radio2.startConstCarrier(RF24_PA_MAX, nextChannel2); // Use RF24_PA_MAX
    }
}