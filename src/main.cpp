#include <Arduino.h>
#include "App.h"

App kivaApp;

void setup() {
  kivaApp.setup();
}

void loop() {
  kivaApp.loop();
}


// #include <Arduino.h>
// #include <NimBLEDevice.h>
// #include <NimBLEServer.h>
// #include <NimBLEHIDDevice.h>
// #include <NimBLEAdvertising.h>

// // For the HID Report Descriptor macros
// #include "HIDTypes.h"

// // Report ID for the keyboard
// #define KEYBOARD_ID 0x01

// // Global pointers for BLE objects
// NimBLEServer*        pServer = nullptr;
// NimBLEHIDDevice*     hid = nullptr;
// NimBLECharacteristic* inputKeyboard = nullptr;
// NimBLEAdvertising*   pAdvertising = nullptr;

// // Define the states for our automated test cycle
// enum TestState {
//   STATE_INITIALIZING,
//   STATE_ADVERTISING,
//   STATE_DISCONNECTING,
//   STATE_DEINITIALIZING,
//   STATE_WAITING_TO_RESTART
// };

// // Use volatile for variables modified in callbacks and used in the loop
// volatile TestState currentState = STATE_INITIALIZING;
// unsigned long stateChangeTimestamp = 0;
// uint16_t      connectionHandle = BLE_HS_CONN_HANDLE_NONE;

// // HID Report Descriptor for a basic Keyboard
// static const uint8_t hidReportDescriptor[] = {
//   USAGE_PAGE(1),      0x01,       // USAGE_PAGE (Generic Desktop)
//   USAGE(1),           0x06,       // USAGE (Keyboard)
//   COLLECTION(1),      0x01,       // COLLECTION (Application)
//   REPORT_ID(1),       KEYBOARD_ID, //   REPORT_ID (1)
//   USAGE_PAGE(1),      0x07,       //   USAGE_PAGE (Keyboard/Keypad)
//   USAGE_MINIMUM(1),   0xE0,       //   USAGE_MINIMUM (Keyboard LeftControl)
//   USAGE_MAXIMUM(1),   0xE7,       //   USAGE_MAXIMUM (Keyboard Right GUI)
//   LOGICAL_MINIMUM(1), 0x00,       //   LOGICAL_MINIMUM (0)
//   LOGICAL_MAXIMUM(1), 0x01,       //   LOGICAL_MAXIMUM (1)
//   REPORT_SIZE(1),     0x01,       //   REPORT_SIZE (1)
//   REPORT_COUNT(1),    0x08,       //   REPORT_COUNT (8)
//   HIDINPUT(1),        0x02,       //   INPUT (Data,Var,Abs) ; Modifier byte
//   REPORT_COUNT(1),    0x01,       //   REPORT_COUNT (1) ; Reserved byte
//   REPORT_SIZE(1),     0x08,       //   REPORT_SIZE (8)
//   HIDINPUT(1),        0x01,       //   INPUT (Cnst,Var,Abs)
//   REPORT_COUNT(1),    0x06,       //   REPORT_COUNT (6) ; 6 key codes
//   REPORT_SIZE(1),     0x08,       //   REPORT_SIZE (8)
//   LOGICAL_MINIMUM(1), 0x00,       //   LOGICAL_MINIMUM (0)
//   LOGICAL_MAXIMUM(1), 0x65,       //   LOGICAL_MAXIMUM (101)
//   USAGE_PAGE(1),      0x07,       //   USAGE_PAGE (Keyboard/Keypad)
//   USAGE_MINIMUM(1),   0x00,       //   USAGE_MINIMUM (Reserved (no event indicated))
//   USAGE_MAXIMUM(1),   0x65,       //   USAGE_MAXIMUM (Keyboard Application)
//   HIDINPUT(1),        0x00,       //   INPUT (Data,Ary,Abs)
//   END_COLLECTION(0)               // END_COLLECTION
// };

// // Standard HID Keyboard Report Structure
// typedef struct {
//   uint8_t modifiers;
//   uint8_t reserved;
//   uint8_t keys[6];
// } KeyReport;


// // Callback class for server events
// class ServerCallbacks: public NimBLEServerCallbacks {
//     void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
//       Serial.printf("[CALLBACK] Client Connected! Peer Address: %s, Conn Handle: %d\n", 
//                     connInfo.getAddress().toString().c_str(), connInfo.getConnHandle());
      
//       connectionHandle = connInfo.getConnHandle();
      
//       delay(500); // Wait a moment for the connection to fully establish
      
//       Serial.println("[KEYPRESS] Sending keystroke: '?'");

//       // Press the '?' key (Shift + /)
//       KeyReport keyReport = {};
//       keyReport.modifiers = 0x02; // Left SHIFT
//       keyReport.keys[0] = 0x38;   // Scancode for '/'
      
//       if(inputKeyboard) {
//         inputKeyboard->notify((uint8_t*)&keyReport, sizeof(keyReport));
//       }

//       delay(20);

//       // Release all keys
//       keyReport = {};
//       if(inputKeyboard) {
//         inputKeyboard->notify((uint8_t*)&keyReport, sizeof(keyReport));
//       }
      
//       Serial.println("[KEYPRESS] Keystroke sent. Triggering disconnect.");
      
//       // Transition state to begin the disconnect process in the main loop
//       currentState = STATE_DISCONNECTING;
//       stateChangeTimestamp = millis();
//     }

//     void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
//       Serial.printf("[CALLBACK] Client Disconnected. Reason: %d. Triggering deinitialization.\n", reason);
//       connectionHandle = BLE_HS_CONN_HANDLE_NONE;
      
//       // Transition state to begin the teardown process
//       currentState = STATE_DEINITIALIZING;
//       stateChangeTimestamp = millis();
//     }
// };


// void setup() {
//   Serial.begin(115200);
//   delay(1000);
//   Serial.println("Starting Automated BLE Keyboard Lifecycle Test...");
//   stateChangeTimestamp = millis();
// }

// void loop() {
//   switch (currentState) {
//     case STATE_INITIALIZING:
//       Serial.println("------------------------------------------");
//       Serial.println("[STATE] INITIALIZING");
      
//       // Initialize the NimBLE stack
//       NimBLEDevice::init("Simple BLE Keyboard");
      
//       // Create the server
//       pServer = NimBLEDevice::createServer();
//       pServer->setCallbacks(new ServerCallbacks());
      
//       // Create the HID device
//       hid = new NimBLEHIDDevice(pServer);
//       inputKeyboard = hid->getInputReport(KEYBOARD_ID);
//       hid->setReportMap((uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor));
//       hid->setManufacturer("Kiva Systems");
//       hid->setPnp(0x02, 0x1234, 0x5678, 0x0110);
//       hid->setHidInfo(0x00, 0x01);
//       hid->startServices();
      
//       // Configure and start advertising
//       pAdvertising = NimBLEDevice::getAdvertising();
//       pAdvertising->setAppearance(HID_KEYBOARD);
//       pAdvertising->addServiceUUID(hid->getHidService()->getUUID());
//       pAdvertising->setName("Simple BLE Keyboard");
//       pAdvertising->start();
      
//       currentState = STATE_ADVERTISING;
//       Serial.println("[STATE] ADVERTISING (Waiting for client...)");
//       Serial.println("------------------------------------------");
//       stateChangeTimestamp = millis();
//       break;

//     case STATE_ADVERTISING:
//       // The onConnect callback will change the state.
//       // This is the idle state while waiting for a connection.
//       break;

//     case STATE_DISCONNECTING:
//       // This state is entered after the keystroke is sent.
//       // We check if a short delay has passed before force-disconnecting.
//       if (millis() - stateChangeTimestamp > 500) {
//         if(connectionHandle != BLE_HS_CONN_HANDLE_NONE) {
//           Serial.println("[STATE] Forcing disconnect...");
//           pServer->disconnect(connectionHandle);
//         } else {
//            // If we somehow got here without a connection, move on
//            currentState = STATE_DEINITIALIZING;
//            stateChangeTimestamp = millis();
//         }
//         // The onDisconnect callback will handle the next state change.
//       }
//       break;

//     case STATE_DEINITIALIZING:
//       // This state is entered from the onDisconnect callback.
//       // Wait a moment before full teardown.
//       if (millis() - stateChangeTimestamp > 500) {
//         Serial.println("[STATE] DEINITIALIZING");
        
//         // Stop advertising (should already be stopped, but good practice)
//         if(pAdvertising && pAdvertising->isAdvertising()) {
//             pAdvertising->stop();
//         }

//         // Clean up the HID device resources
//         if (hid) {
//             NimBLEService* hidService = hid->getHidService();
//             NimBLEService* deviceInfoService = hid->getDeviceInfoService();
//             NimBLEService* batteryService = hid->getBatteryService();

//             if (hidService) pServer->removeService(hidService, true);
//             if (deviceInfoService) pServer->removeService(deviceInfoService, true);
//             if (batteryService) pServer->removeService(batteryService, true);

//             delete hid;
//             hid = nullptr;
//         }

//         // Fully de-initialize the BLE stack
//         if(NimBLEDevice::deinit(true)) {
//             Serial.println("Stack de-initialized successfully.");
//         } else {
//             Serial.println("Stack de-init failed.");
//         }
        
//         currentState = STATE_WAITING_TO_RESTART;
//         Serial.println("[STATE] WAITING TO RESTART (5 seconds)...");
//         stateChangeTimestamp = millis();
//       }
//       break;

//     case STATE_WAITING_TO_RESTART:
//       if (millis() - stateChangeTimestamp > 5000) {
//         Serial.println("\n>>> RESTARTING CYCLE <<<\n");
//         currentState = STATE_INITIALIZING;
//       }
//       break;
//   }
  
//   delay(10);
// }