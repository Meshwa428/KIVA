#include "wifi_attack_tools.h"
#include "wifi_manager.h"
#include "sd_card_manager.h"
#include <vector>

#define MAX_SSID_LIST_SIZE 100
#define BEACON_SSID_FILE_PATH "/system/beacon_ssids.txt"
#define RICK_ROLL_SSID_FILE_PATH "/system/rick_roll_ssids.txt"

// --- Structures and constants for the attack ---
struct FakeAP {
  String ssid;
  uint8_t bssid[6];
  bool is_wpa2; // NEW: Flag to determine if this AP is open or WPA2
};

// --- Module-specific static variables ---
static uint32_t attackTime = 0;
static const uint32_t attackDelay = 10;
static std::vector<FakeAP> fake_aps_list;
static int current_fake_ap_index = 0;
static BeaconSsidMode current_ssid_mode = BeaconSsidMode::UNINITIALIZED;
static std::vector<String> ssid_list_from_file;

// --- BEACON PACKET TEMPLATES ---

// Template for an OPEN (unlocked) network
static uint8_t open_beacon_template[] = {
    0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Frame Control & Destination
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,                         // BSSID (placeholder)
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,                         // Source (placeholder)
    0x00, 0x00,                                                 // Sequence Number
    // --- Fixed Parameters ---
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,             // Timestamp
    0x64, 0x00,                                                 // Beacon Interval
    0x01, 0x04,                                                 // Capability Info: ESS, IBSS - ** OPEN NETWORK **
    // --- Tagged Parameters ---
    0x00, 0x00                                                  // Tag: SSID, Length (placeholder)
};
// Data that comes after the SSID tag in an OPEN beacon
static uint8_t open_beacon_post_ssid_tags[] = {
    0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // Supported Rates
    0x03, 0x01, 0x01                                            // DS Set (Channel)
};


// Template for a WPA2-PSK (locked) network
static uint8_t wpa2_beacon_template[] = {
    0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Frame Control & Destination
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,                         // BSSID (placeholder)
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,                         // Source (placeholder)
    0x00, 0x00,                                                 // Sequence Number
    // --- Fixed Parameters ---
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,             // Timestamp
    0x64, 0x00,                                                 // Beacon Interval
    0x31, 0x04,                                                 // Capability Info: ESS, IBSS, ** Privacy (WPA) **
    // --- Tagged Parameters ---
    0x00, 0x00                                                  // Tag: SSID, Length (placeholder)
};
// Data that comes after the SSID tag in a WPA2 beacon
static uint8_t wpa2_beacon_post_ssid_tags[] = {
    // Supported Rates
    0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c,
    // DS Set (Channel)
    0x03, 0x01, 0x01,
    // RSN (Robust Security Network) Information Element for WPA2-PSK
    0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00,
    0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02,
    0x00, 0x00
};


static void send_beacon_packet(const FakeAP& ap, uint8_t channel) {
  const uint8_t* template_data;
  const uint8_t* post_ssid_tags_data;
  size_t template_len;
  size_t post_tags_len;

  // Choose the correct template based on the AP's security flag
  if (ap.is_wpa2) {
    template_data = wpa2_beacon_template;
    template_len = sizeof(wpa2_beacon_template);
    post_ssid_tags_data = wpa2_beacon_post_ssid_tags;
    post_tags_len = sizeof(wpa2_beacon_post_ssid_tags);
  } else {
    template_data = open_beacon_template;
    template_len = sizeof(open_beacon_template);
    post_ssid_tags_data = open_beacon_post_ssid_tags;
    post_tags_len = sizeof(open_beacon_post_ssid_tags);
  }

  uint8_t ssid_len = ap.ssid.length();
  if (ssid_len > 32) ssid_len = 32;

  size_t total_len = template_len + ssid_len + post_tags_len;
  uint8_t packet[total_len];

  memcpy(packet, template_data, template_len);
  memcpy(&packet[10], ap.bssid, 6);
  memcpy(&packet[16], ap.bssid, 6);
  
  // Set SSID length tag and copy SSID name
  packet[template_len - 1] = ssid_len;
  memcpy(&packet[template_len], ap.ssid.c_str(), ssid_len);

  // Copy post-SSID tags and set the channel
  uint8_t* post_tags_ptr = &packet[template_len + ssid_len];
  memcpy(post_tags_ptr, post_ssid_tags_data, post_tags_len);
  post_tags_ptr[post_tags_len - 1] = channel;

  esp_wifi_80211_tx(WIFI_IF_STA, packet, total_len, false);
}

// Helper function to generate a random SSID
static void generate_random_ssid(char* buffer, size_t len) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
    int ssid_len = 8 + (esp_random() % 17);
    if (ssid_len >= len) ssid_len = len - 1;
    for (int i = 0; i < ssid_len; ++i) {
        buffer[i] = charset[esp_random() % (sizeof(charset) - 1)];
    }
    buffer[ssid_len] = '\0';
}


// --- Public Control Functions ---
void start_beacon_spam(bool isRickRollMode, int channel) {
  if (isBeaconSpamActive || isRickRollActive || isJammingOperationActive) {
    Serial.println("AttackTools: Another RF operation is already active.");
    return;
  }
  const char* modeName = isRickRollMode ? "Rick Roll Spam" : "Beacon Spam";
  Serial.printf("AttackTools: Starting %s on channel %d.\n", modeName, channel);

  // --- Load SSIDs from the appropriate file ---
  // This part correctly selects rick_roll_ssids.txt if isRickRollMode is true
  const char* ssidFilePath = isRickRollMode ? RICK_ROLL_SSID_FILE_PATH : BEACON_SSID_FILE_PATH;
  ssid_list_from_file.clear();
  if (isSdCardAvailable() && SD.exists(ssidFilePath)) {
    String fileContent = readSdFile(ssidFilePath);
    if (fileContent.length() > 0) {
      int currentIndex = 0;
      while (currentIndex < fileContent.length() && ssid_list_from_file.size() < MAX_SSID_LIST_SIZE) {
        int next_newline = fileContent.indexOf('\n', currentIndex);
        if (next_newline == -1) next_newline = fileContent.length();
        String line = fileContent.substring(currentIndex, next_newline);
        line.trim();
        if (line.length() > 0 && line.length() <= 32 && !line.startsWith("//") && !line.startsWith("#")) {
          ssid_list_from_file.push_back(line);
        }
        currentIndex = next_newline + 1;
      }
    }
  }

  // Determine mode based on whether SSIDs were loaded
  if (ssid_list_from_file.empty()) {
    // Abort Rick Roll if its specific SSID file is missing/empty
    if (isRickRollMode) {
      Serial.println("AttackTools: Rick Roll mode requires SSIDs. Aborting.");
      return;
    }
    current_ssid_mode = BeaconSsidMode::RANDOM;
    Serial.println("SSID Manager: Using random SSID mode.");
  } else {
    current_ssid_mode = BeaconSsidMode::FROM_FILE;
    Serial.printf("SSID Manager: Loaded %d SSIDs from %s.\n", ssid_list_from_file.size(), ssidFilePath);
  }

  // --- Populate the list of fake APs ---
  fake_aps_list.clear();
  if (current_ssid_mode == BeaconSsidMode::FROM_FILE) {
    // This block handles both normal Beacon Spam and Rick Roll from their respective files.
    Serial.printf("AttackTools: Creating %d fake APs based on SD card list.\n", ssid_list_from_file.size());
    for (const auto& ssid_name : ssid_list_from_file) {
      FakeAP ap;
      ap.ssid = ssid_name;
      
      // EXPLICIT CONFIRMATION: This randomization applies to Rick Roll SSIDs too.
      // We randomly choose if the network is open or WPA2 protected.
      ap.is_wpa2 = random(0, 2) == 0; 

      for (int j = 0; j < 6; j++) {
        ap.bssid[j] = esp_random() & 0xFF;
      }
      ap.bssid[0] &= 0xFE;
      ap.bssid[0] |= 0x02;
      fake_aps_list.push_back(ap);
    }
  } else { // This block only runs for normal Beacon Spam if its file is empty.
    const int NUM_RANDOM_APS = 15;
    Serial.printf("AttackTools: Creating %d fake APs with random names.\n", NUM_RANDOM_APS);
    for (int i = 0; i < NUM_RANDOM_APS; i++) {
        FakeAP ap;
        static char random_buffer[33];
        generate_random_ssid(random_buffer, sizeof(random_buffer));
        ap.ssid = random_buffer;
        ap.is_wpa2 = random(0, 2) == 0;
        for (int j = 0; j < 6; j++) {
          ap.bssid[j] = esp_random() & 0xFF;
        }
        ap.bssid[0] &= 0xFE;
        ap.bssid[0] |= 0x02;
        fake_aps_list.push_back(ap);
    }
  }

  if (fake_aps_list.empty()) {
    Serial.println("AttackTools: Failed to generate any fake APs. Aborting.");
    return;
  }
  
  // Enable Wi-Fi for injection
  setWifiHardwareState(true, RF_MODE_WIFI_SNIFF_PROMISC, channel);
  if (!wifiHardwareEnabled || currentRfMode != RF_MODE_WIFI_SNIFF_PROMISC) {
    Serial.println("AttackTools: Failed to enable Wi-Fi for injection.");
    return;
  }

  esp_wifi_set_max_tx_power(82);
  attackTime = 0;
  current_fake_ap_index = 0;
  
  if (isRickRollMode) { isRickRollActive = true; } 
  else { isBeaconSpamActive = true; }
  
  currentInputPollInterval = INPUT_POLL_INTERVAL_JAMMING;
  currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_JAMMING;
}

void stop_beacon_spam() {
  if (!isBeaconSpamActive && !isRickRollActive) return;
  Serial.println("AttackTools: Stopping all beacon attacks.");
  isBeaconSpamActive = false;
  isRickRollActive = false;
  fake_aps_list.clear();
  setWifiHardwareState(false);
  currentInputPollInterval = INPUT_POLL_INTERVAL_NORMAL;
  currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_NORMAL;
}

void run_beacon_spam_cycle() {
  if (!isBeaconSpamActive && !isRickRollActive) return;
  if (!wifiHardwareEnabled || fake_aps_list.empty()) return;

  uint32_t currentTime = millis();
  if (currentTime - attackTime > attackDelay) {
    uint8_t current_channel;
    wifi_second_chan_t second;
    if (esp_wifi_get_channel(&current_channel, &second) != ESP_OK) return;

    // Get the current fake AP from our list
    const FakeAP& ap_to_spam = fake_aps_list[current_fake_ap_index];

    // Send the beacon packet (open or WPA2)
    send_beacon_packet(ap_to_spam, current_channel);

    // Move to the next fake AP in the list for the next cycle
    current_fake_ap_index = (current_fake_ap_index + 1) % fake_aps_list.size();

    // Randomly hop channel occasionally
    if ((esp_random() % 100) < 5) {
      uint8_t new_channel = 1 + (esp_random() % 11);
      if (new_channel != current_channel) {
        esp_wifi_set_channel(new_channel, WIFI_SECOND_CHAN_NONE);
      }
    }

    attackTime = currentTime;
  }
}

BeaconSsidMode get_beacon_ssid_mode() {
  return current_ssid_mode;
}