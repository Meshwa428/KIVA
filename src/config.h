#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "keyboard_layout.h" // Make sure KeyboardLayer is known
#include "jamming.h" // <--- INCLUDE jamming.h HERE


// === Device Settings ===
#define DEVICE_HOSTNAME "KivaDevice" // <<< YOUR GLOBAL DEVICE HOSTNAME HERE

// === NRF24L01 Jamming Pins & Config === <--- NEW SECTION
#define NRF1_CE_PIN 1
#define NRF1_CSN_PIN 2 
#define NRF2_CE_PIN 43 
#define NRF2_CSN_PIN 44
#define SPI_SPEED_NRF 16000000 // SPI speed for NRF modules


// === I2C Multiplexer & Peripherals ===
#define MUX_ADDR 0x70
#define PCF0_ADDR 0x24
#define PCF1_ADDR 0x20

// --- Display MUX Channels ---
#define MUX_CHANNEL_MAIN_DISPLAY 4
#define MUX_CHANNEL_SECOND_DISPLAY 2

// --- Second Display I2C Address ---
#define SECOND_DISPLAY_I2C_ADDR 0x3C

// === Display ===
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// === ADC & Battery ===
#define ADC_PIN 3
#define ADC_REF 3.3f
#define ADC_RES 4095
#define VOLTAGE_RATIO (70.0f / 50.0f)
#define BAT_SAMPLES 20
#define BATTERY_CHECK_INTERVAL 1000UL         // Original, can be fallback
#define BATTERY_CHECK_INTERVAL_NORMAL 1000UL  // ms
#define BATTERY_CHECK_INTERVAL_JAMMING 5000UL // ms


// === Buttons (PCF0) ===
#define ENC_BTN 0
#define ENC_A 1
#define ENC_B 2
#define BTN_AI 3
#define BTN_RIGHT1 4
#define BTN_RIGHT2 5
#define LASER_PIN_PCF0 6
#define MOTOR_PIN_PCF0 7


// === Navigation Buttons (PCF1) ===
#define NAV_OK 0
#define NAV_BACK 1
#define NAV_A 2
#define NAV_B 3
#define NAV_LEFT 4
#define NAV_UP 5
#define NAV_DOWN 6
#define NAV_RIGHT 7

// === Debouncing & Auto-repeat ===
#define DEBOUNCE_DELAY 50UL // ms
#define REPEAT_INIT_DELAY 400UL // ms
#define REPEAT_INT 150UL // ms
#define INPUT_POLL_INTERVAL_NORMAL 8UL     // ms <--- MAKE SURE THIS IS PRESENT
#define INPUT_POLL_INTERVAL_JAMMING 2000UL  // ms <--- MAKE SURE THIS IS PRESENT
#define JAMMER_CYCLE_DELAY_MS 1             // ms, delay in jamming loop <--- MAKE SURE THIS IS PRESENT

// === Menu System ===
enum MenuState {
  MAIN_MENU,
  GAMES_MENU,
  TOOLS_MENU,
  SETTINGS_MENU,
  UTILITIES_MENU,
  TOOL_CATEGORY_GRID,
  WIFI_SETUP_MENU,
  FLASHLIGHT_MODE,
  WIFI_PASSWORD_INPUT,
  WIFI_CONNECTING,
  WIFI_CONNECTION_INFO,
  JAMMING_ACTIVE_SCREEN // <--- NEW
  // WIFI_DISCONNECT_OVERLAY, // Not using a state, using a flag instead
};

// --- Extern Global Variables ---
extern MenuState currentMenu;
extern int menuIndex;
extern int maxMenuItems;
extern int mainMenuSavedIndex;
extern int toolsCategoryIndex;
extern int gridCols;
extern int targetGridScrollOffset_Y;
extern float currentGridScrollOffset_Y_anim;
extern bool isCharging;


extern bool vibrationOn;
extern bool laserOn;
extern bool wifiHardwareEnabled;

extern uint8_t pcf0Output;

// --- NEW EXTERN DECLARATIONS FOR INTERVALS ---
extern unsigned long currentBatteryCheckInterval;
extern unsigned long currentInputPollInterval;
extern int lastSelectedJammingToolGridIndex; // <--- NEW EXTERN

// --- UI Layout Constants ---
#define GRID_ITEM_H 18
#define GRID_ITEM_PADDING_Y 4
#define GRID_ITEM_PADDING_X 4
#define STATUS_BAR_H 11
#define WIFI_LIST_ITEM_H 18

#define PASSWORD_INPUT_FIELD_Y 30
#define PASSWORD_INPUT_FIELD_H 12
#define PASSWORD_MAX_LEN 63


// === SD Card ===
#define SD_CS_PIN 21


// --- Wi-Fi Related Globals & Struct ---
#define MAX_WIFI_NETWORKS 15
#define WIFI_SCAN_CHECK_INTERVAL 250UL
#define WIFI_CONNECTION_TIMEOUT_MS 15000UL

struct WifiNetwork {
  char ssid[33];
  int8_t rssi;
  bool isSecure;
};
extern WifiNetwork scannedNetworks[MAX_WIFI_NETWORKS];
extern int foundWifiNetworksCount;
extern int wifiMenuIndex;
extern bool wifiIsScanning;
extern unsigned long lastWifiScanCheckTime;
extern float currentWifiListScrollOffset_Y_anim;
extern int targetWifiListScrollOffset_Y;

extern char currentSsidToConnect[33];
extern char wifiPasswordInput[PASSWORD_MAX_LEN + 1];
extern int wifiPasswordInputCursor;
extern bool selectedNetworkIsSecure;

// --- Wi-Fi Disconnect Overlay ---
extern bool showWifiDisconnectOverlay;    // <--- NEW
extern int disconnectOverlaySelection; // <--- NEW (0 for Cancel, 1 for Confirm)

// --- Wi-Fi Disconnect Overlay Animation ---
extern float disconnectOverlayCurrentScale;
extern float disconnectOverlayTargetScale;
extern unsigned long disconnectOverlayAnimStartTime;
extern bool disconnectOverlayAnimatingIn;


// --- Keyboard related globals ---
extern KeyboardLayer currentKeyboardLayer;
extern int keyboardFocusRow;
extern int keyboardFocusCol;
extern bool capsLockActive;

// --- Animation Constants & Structs ---
#define MAX_ANIM_ITEMS (MAX_WIFI_NETWORKS + 2)
#define MAX_GRID_ITEMS 20
#define GRID_ANIM_STAGGER_DELAY 40.0f
#define GRID_ANIM_SPEED 20.0f

extern float gridItemScale[MAX_GRID_ITEMS];
extern float gridItemTargetScale[MAX_GRID_ITEMS];
extern unsigned long gridItemAnimStartTime[MAX_GRID_ITEMS];
extern bool gridAnimatingIn;


struct QuadratureEncoder {
  int position;
  int lastState;
  unsigned long lastValidTime;
  int consecutiveValid;
  const unsigned long minInterval = 5;
  const int requiredConsecutive = 2;
  const int cwTable[4] = {1, 3, 0, 2};
  const int ccwTable[4] = {2, 0, 3, 1};

  void init(int iA, int iB);
  bool update(int cA, int cB);
};

struct VerticalListAnimation {
  float itemOffsetY[MAX_ANIM_ITEMS], itemScale[MAX_ANIM_ITEMS];
  float targetOffsetY[MAX_ANIM_ITEMS], targetScale[MAX_ANIM_ITEMS];
  
  float introStartSourceOffsetY[MAX_ANIM_ITEMS]; 
  float introStartSourceScale[MAX_ANIM_ITEMS]; 
  bool isIntroPhase; 
  unsigned long introStartTime;
  
  // --- MODIFIED VALUES ---
  static constexpr float animSpd = 12.f; // Made static constexpr for in-class init
  static constexpr float frmTime = 0.016f;
  static constexpr float itmSpc = 18.f;      // WAS 20.f - Key change for clipping
  static constexpr float introDuration = 500.0f; // WAS 350.0f - For smoother new intro
  // --- END MODIFIED VALUES ---


  void init();
  // void startIntro(int selIdx, int total, float commonInitialYOffset, float commonInitialScale); // Old signature
  void startIntro(int selIdx, int total); // New signature for enhanced intro
  void setTargets(int selIdx, int total);
  bool update();
};

struct CarouselAnimation {
  float itemOffsetX[MAX_ANIM_ITEMS], itemScale[MAX_ANIM_ITEMS];
  float targetOffsetX[MAX_ANIM_ITEMS], targetScale[MAX_ANIM_ITEMS];
  const float animSpd = 12.f, frmTime = 0.016f;
  const float cardBaseW = 58.f, cardBaseH = 42.f, cardGap = 6.f;

  void init();
  void setTargets(int selIdx, int total);
  bool update();
};

// --- Marquee Scroll ---
#define MARQUEE_SCROLL_SPEED 1
#define MARQUEE_UPDATE_INTERVAL 70UL
#define MARQUEE_PAUSE_DURATION 1200UL

extern char marqueeText[40];
extern int marqueeTextLenPx;
extern float marqueeOffset;
extern unsigned long lastMarqueeTime;
extern bool marqueeActive;
extern bool marqueePaused;
extern unsigned long marqueePauseStartTime;
extern bool marqueeScrollLeft;

extern VerticalListAnimation mainMenuAnim;
extern CarouselAnimation subMenuAnim;
extern VerticalListAnimation wifiListAnim;

// --- Shared Buffer for truncateText ---
extern char SBUF[32];


#endif // CONFIG_H