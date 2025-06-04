#include "jamming.h"
#include "esp_bt.h"     // For esp_bt_controller_deinit/init
#include "esp_wifi.h"   // For esp_wifi_stop/deinit/init/start
#include <SPI.h>        // For SPI communication with NRF24

// NRF24L01 radio objects
// Pins should be defined in config.h and passed here or used directly
RF24 radio1(NRF1_CE_PIN, NRF1_CSN_PIN, SPI_SPEED_NRF);
RF24 radio2(NRF2_CE_PIN, NRF2_CSN_PIN, SPI_SPEED_NRF);

// Channel lists
// Note: NRF24L01 channels are 0-125, roughly 2400MHz to 2525MHz.
// Bluetooth/BLE channels are mapped within this range.
// For simplicity, we'll use the NRF channel numbers that correspond to BLE/BT advertising/data channels.

// Approximate NRF channels for BLE advertising (actual frequencies: 2402, 2426, 2480 MHz)
// Channel = Freq_MHz - 2400
const int ble_adv_nrf_channels[] = {2, 26, 80}; 

// A selection of NRF channels for general BT classic (BT hops across 79 channels from 2402 to 2480 MHz)
// This is a subset for faster hopping example.
const int bt_classic_nrf_channels[] = {
    2,  5,  8, 11, 14, 17, 20, 23, 26, 29, // Approx 2402-2429 MHz
    32, 35, 38, 41, 44, 47, 50, 53, 56, 59, // Approx 2432-2459 MHz
    62, 65, 68, 71, 74, 77, 80              // Approx 2462-2480 MHz
}; 

// For NRF_CHANNELS mode, we can iterate 0-125
// For RF_NOISE_FLOOD, we can rapidly hop 0-125

static int current_ble_channel_idx = 0;
static int current_bt_channel_idx = 0;
static int current_nrf_channel_sweep = 0;

bool configureRadioForJamming(RF24 &radio, const char* radioName) {
    Serial.printf("Jamming: Initializing %s... ", radioName);
    if (radio.begin(&SPI)) { // Pass SPIClass instance if not default
        radio.setAutoAck(false);
        radio.stopListening();
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
    // SPI.begin() is usually called in main setup.
    // If NRFs are on a different SPI bus, initialize it here.
    Serial.println("Jamming: System setup complete (NRF init deferred to startActiveJamming).");
}

bool startActiveJamming(JammingType type) {
    if (isJammingOperationActive) {
        Serial.println("Jamming: Already active. Stopping current then starting new.");
        stopActiveJamming(); // Stop first
        delay(100); // Small delay
    }

    Serial.printf("Jamming: Attempting to start mode %d.\n", type);
    activeJammingType = type;

    // De-initialize ESP32's own radios
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
        return false;
    }
    if (!radio1_ok) Serial.println("Jamming: Warning - NRF24L01 #1 failed, proceeding with #2 only.");
    if (!radio2_ok) Serial.println("Jamming: Warning - NRF24L01 #2 failed, proceeding with #1 only.");


    // Start constant carrier on an initial channel. runJammerCycle will hop.
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
            return false;
    }

    if (radio1_ok) radio1.startConstCarrier(RF24_PA_HIGH, initialChannel);
    if (radio2_ok) radio2.startConstCarrier(RF24_PA_HIGH, initialChannel);
    
    isJammingOperationActive = true;
    Serial.printf("Jamming: Mode %d started. Initial channel: %d\n", type, initialChannel);
    return true;
}

void stopActiveJamming() {
    Serial.println("Jamming: Stopping active jamming...");
    if (radio1.isChipConnected()) radio1.stopConstCarrier();
    if (radio2.isChipConnected()) radio2.stopConstCarrier();
    
    // Power down radios
    // radio1.powerDown(); // powerDown() might not be necessary if chip select is managed
    // radio2.powerDown(); // or if ESP32 radio re-init handles bus state.
                        // For now, stopConstCarrier should suffice.

    activeJammingType = JAM_NONE;
    isJammingOperationActive = false;

    // Re-initialize ESP32's own Wi-Fi (Bluetooth re-init is more complex and depends on usage)
    // For now, user will have to manually re-enable Wi-Fi via Kiva menus if they need it.
    // This simplifies state management significantly.
    Serial.println("Jamming: ESP32 BT/Wi-Fi controllers remain de-initialized.");
    Serial.println("Jamming: To use Wi-Fi, re-enable via Kiva settings.");
    // If automatic re-initialization is desired:
    // Serial.println("Jamming: Re-initializing ESP32 Wi-Fi controller...");
    // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // esp_wifi_init(&cfg);
    // esp_wifi_set_mode(WIFI_MODE_STA); // Or whatever mode Kiva usually uses
    // esp_wifi_start();
    // Serial.println("Jamming: ESP32 Wi-Fi controller re-initialized.");

    Serial.println("Jamming: Stopped.");
}

void runJammerCycle() {
    if (!isJammingOperationActive) return;

    int nextChannel1 = -1, nextChannel2 = -1; // Use -1 to indicate no change needed for a radio

    switch (activeJammingType) {
        case JAM_BLE:
            current_ble_channel_idx = (current_ble_channel_idx + 1) % (sizeof(ble_adv_nrf_channels) / sizeof(ble_adv_nrf_channels[0]));
            nextChannel1 = ble_adv_nrf_channels[current_ble_channel_idx];
            // Optional: use radio2 for a different BLE channel or same for more power on one.
            // For simplicity, both radios on the same channel here.
            nextChannel2 = ble_adv_nrf_channels[current_ble_channel_idx];
            break;

        case JAM_BT:
            current_bt_channel_idx = (current_bt_channel_idx + 1) % (sizeof(bt_classic_nrf_channels) / sizeof(bt_classic_nrf_channels[0]));
            nextChannel1 = bt_classic_nrf_channels[current_bt_channel_idx];
            // Stagger radio2 slightly or use a different hop pattern for broader coverage
            // For now, same channel.
            nextChannel2 = bt_classic_nrf_channels[current_bt_channel_idx]; 
            break;

        case JAM_NRF_CHANNELS: // Slower sweep through all NRF channels
            current_nrf_channel_sweep = (current_nrf_channel_sweep + 1) % 126; // 0-125
            nextChannel1 = current_nrf_channel_sweep;
            nextChannel2 = current_nrf_channel_sweep; // Or (current_nrf_channel_sweep + X) % 126
            // Add a small delay here if a "slower" sweep is desired for this mode specifically
            // delayMicroseconds(500); // Example: 0.5ms per channel
            break;

        case JAM_RF_NOISE_FLOOD: // Fast random hop across many channels
            nextChannel1 = random(0, 126); // Target NRF channels 0-125
            nextChannel2 = random(0, 126); // Can be different for wider instant spectrum coverage
            break;

        default:
            return; // Not an active jamming mode
    }

    // Update radio channels if they are valid and the radio is connected
    if (nextChannel1 != -1 && radio1.isChipConnected()) {
        radio1.setChannel(nextChannel1); 
        // radio1.startConstCarrier() is not needed again if setChannel retains CW mode.
        // If not, then: radio1.startConstCarrier(RF24_PA_HIGH, nextChannel1);
        // The library typically requires re-calling startConstCarrier if you change channel
        // *after* it has been started. However, setChannel then start is fine.
        // To be safe, we can call startConstCarrier again.
        radio1.startConstCarrier(RF24_PA_HIGH, nextChannel1);

    }
    if (nextChannel2 != -1 && radio2.isChipConnected()) {
        radio2.setChannel(nextChannel2);
        radio2.startConstCarrier(RF24_PA_HIGH, nextChannel2);
    }
    
    // A very short delay can allow the radio to settle on the new channel,
    // but for rapid jamming, minimizing delay is often preferred.
    // delayMicroseconds(50); // Optional: 50 microseconds
}