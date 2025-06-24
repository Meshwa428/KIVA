#include <Wire.h>
#include "config.h"
#include "keyboard_layout.h"
#include "pcf_utils.h"
#include "input_handling.h"
#include "battery_monitor.h"
#include "ui_drawing.h"
#include "menu_logic.h"
#include "wifi_manager.h"
#include "sd_card_manager.h"
#include "jamming.h"
#include "ota_manager.h"
#include "firmware_metadata.h"  // <--- NEW INCLUDE

// For OTA Web Server and Basic OTA
#include <WebServer.h>  // Already in config.h for extern WebServer
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Update.h>  // For Update class, used by ota_manager

// Bitmap Data
#define im_width 128
#define im_height 64

static const unsigned char im_bits[] U8X8_PROGMEM = {  // Added U8X8_PROGMEM to save RAM
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xe0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x1f, 0x00, 0x00, 0x00, 0x20,
  0x00, 0x00, 0x00, 0x00, 0xe2, 0x1f, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x18, 0x00, 0x00,
  0x38, 0x00, 0x00, 0x00, 0x80, 0xf1, 0x1f, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x07, 0x1c, 0x00,
  0x00, 0x3f, 0x00, 0x00, 0x00, 0xe0, 0xf0, 0x2f, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x07, 0x1e,
  0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0x27, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x03,
  0x1f, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x7c, 0xf0, 0x47, 0x00, 0x00, 0x00, 0x00, 0xff,
  0x81, 0x3f, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x7f, 0xf0, 0xc7, 0x00, 0x00, 0x00, 0x80,
  0xff, 0xc0, 0x3f, 0x00, 0x80, 0x1f, 0x00, 0x00, 0x80, 0x3f, 0xf8, 0xc7, 0x00, 0x00, 0x00,
  0xc0, 0x7f, 0xe0, 0x3f, 0x00, 0x80, 0x1f, 0x00, 0x00, 0xe0, 0x3f, 0xf8, 0x83, 0x01, 0x00,
  0x00, 0xe0, 0x3f, 0xf0, 0x7f, 0x00, 0xc0, 0x1f, 0x00, 0x00, 0xf8, 0x1f, 0xf8, 0x83, 0x03,
  0x00, 0x00, 0xe0, 0x1f, 0xf8, 0x7f, 0x00, 0xc0, 0x1f, 0x00, 0x00, 0xfe, 0x1f, 0xf8, 0x03,
  0x03, 0x00, 0x00, 0xf0, 0x0f, 0xfc, 0x7f, 0x00, 0xc0, 0x1f, 0x00, 0x00, 0xff, 0x0f, 0xf8,
  0x03, 0x07, 0x00, 0x00, 0xf8, 0x07, 0xfe, 0xfd, 0x00, 0xc0, 0x1f, 0x00, 0xc0, 0xff, 0x07,
  0xfc, 0x03, 0x0f, 0x00, 0x00, 0xfc, 0x03, 0xff, 0xfc, 0x00, 0xc0, 0x1f, 0x00, 0xf0, 0xff,
  0x01, 0xfc, 0x01, 0x0e, 0x00, 0x00, 0xfe, 0x83, 0x7f, 0xfc, 0x00, 0xe0, 0x0f, 0x00, 0xfc,
  0x7f, 0x00, 0xfc, 0x01, 0x1e, 0x00, 0x00, 0xff, 0xc1, 0x3f, 0xf8, 0x01, 0xe0, 0x0f, 0x00,
  0xfe, 0x1f, 0x00, 0xfc, 0x01, 0x3c, 0x00, 0x80, 0xff, 0xf0, 0x1f, 0xf8, 0x01, 0xe0, 0x0f,
  0x80, 0xff, 0x07, 0x00, 0xfc, 0x01, 0x3c, 0x00, 0xc0, 0x7f, 0xf8, 0x0f, 0xf8, 0x01, 0xe0,
  0x0f, 0xe0, 0xff, 0x01, 0x00, 0xfe, 0x00, 0x7c, 0x00, 0xe0, 0x3f, 0xfc, 0x07, 0xf8, 0x03,
  0xf0, 0x0f, 0xf0, 0x7f, 0x00, 0x00, 0xfe, 0x00, 0xf8, 0x00, 0xf0, 0x1f, 0xfe, 0x03, 0xf0,
  0x03, 0xf0, 0x0f, 0xfc, 0x1f, 0x00, 0x00, 0xfe, 0x00, 0xf8, 0x00, 0xf8, 0x0f, 0xff, 0x01,
  0xf0, 0x03, 0xf0, 0x0f, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x00, 0xf8, 0x01, 0xfc, 0x87, 0xff,
  0x00, 0xf0, 0x07, 0xf0, 0xcf, 0xff, 0x01, 0x00, 0x00, 0xfe, 0x00, 0xf0, 0x03, 0xfe, 0xc3,
  0x7f, 0x00, 0xf0, 0x07, 0xf0, 0xe7, 0x7f, 0x00, 0x00, 0x00, 0x7f, 0x00, 0xf0, 0x03, 0xff,
  0xe1, 0x1f, 0x00, 0xe0, 0x07, 0xf8, 0xff, 0xff, 0x01, 0x00, 0x00, 0x7f, 0x00, 0xe0, 0x87,
  0xff, 0xf1, 0x0f, 0x00, 0xe0, 0x07, 0xf8, 0xe7, 0xff, 0x07, 0x00, 0x00, 0x7f, 0x00, 0xe0,
  0xcf, 0xff, 0xf8, 0x07, 0x00, 0xe0, 0x0f, 0xf8, 0x07, 0xff, 0x0f, 0x00, 0x00, 0x7f, 0x00,
  0xe0, 0xef, 0x7f, 0xfc, 0x03, 0x00, 0xe0, 0x0f, 0xf8, 0x07, 0xfc, 0x3f, 0x00, 0x00, 0x3f,
  0x00, 0xc0, 0xff, 0x3f, 0xfe, 0x01, 0x00, 0xc0, 0x0f, 0xf8, 0x07, 0xe0, 0xff, 0x00, 0x80,
  0x3f, 0x00, 0xc0, 0xff, 0x1f, 0xff, 0x00, 0x00, 0xfc, 0x1f, 0xfc, 0x07, 0x00, 0xff, 0x01,
  0x80, 0x3f, 0x00, 0x80, 0xff, 0xcf, 0x7f, 0xfe, 0xff, 0xc7, 0x1f, 0xfc, 0x07, 0x00, 0xfc,
  0x07, 0x80, 0x3f, 0x00, 0x80, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xc0, 0x1f, 0xfc, 0x03, 0x00,
  0xe0, 0x1f, 0x00, 0x3f, 0x00, 0x80, 0xff, 0xe3, 0xff, 0xff, 0x0f, 0x80, 0x3f, 0xfe, 0x03,
  0x00, 0x00, 0x7f, 0x00, 0x1f, 0x00, 0x00, 0xff, 0xc1, 0xff, 0xff, 0x01, 0x80, 0x3f, 0xfe,
  0x03, 0x00, 0x00, 0xf8, 0x00, 0x1f, 0x00, 0x00, 0xff, 0x80, 0xff, 0x3f, 0x00, 0x80, 0x3f,
  0xfc, 0x03, 0x00, 0x00, 0xe0, 0x03, 0x1e, 0x00, 0x00, 0xff, 0x00, 0xff, 0x07, 0x00, 0x80,
  0x3f, 0xf8, 0x03, 0x00, 0x00, 0x00, 0x0f, 0x1e, 0x00, 0x00, 0x7e, 0x00, 0xfe, 0x00, 0x00,
  0x00, 0x3f, 0xe0, 0x03, 0x00, 0x00, 0x00, 0x38, 0x0e, 0x00, 0x00, 0x3e, 0x00, 0x0c, 0x00,
  0x00, 0x00, 0x1f, 0xc0, 0x03, 0x00, 0x00, 0x00, 0xe0, 0x0c, 0x00, 0x00, 0x1c, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x1f, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x0c, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x04,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// === Global Object Definitions ===
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2_small(U8G2_R0, U8X8_PIN_NONE);
QuadratureEncoder encoder;  // Definition
VerticalListAnimation mainMenuAnim;
CarouselAnimation subMenuAnim;
VerticalListAnimation wifiListAnim;
WebServer otaWebServer(OTA_WEB_SERVER_PORT);  // Unchanged

FirmwareInfo g_firmwareToInstallFromSd;

// === Global Variable Definitions ===
MenuState currentMenu = MAIN_MENU;
MenuState postWifiActionMenu = currentMenu;  // Stores the menu to go to after Wi-Fi connect
ActiveRfOperationMode currentRfMode = RF_MODE_OFF; // <--- DEFINITION
bool wifiConnectForScheduledAction = false;
int menuIndex = 0;
int maxMenuItems = 0;
int mainMenuSavedIndex = 0;
int toolsCategoryIndex = 0;
int gridCols = 2;
int targetGridScrollOffset_Y = 0;
float currentGridScrollOffset_Y_anim = 0.0f;
uint8_t pcf0Output = 0xFF & ~((1 << LASER_PIN_PCF0) | (1 << MOTOR_PIN_PCF0));  // Ensure LASER and MOTOR pins are off initially

float batReadings[BAT_SAMPLES] = { 0 };
int batIndex = 0;
bool batInitialized = false;
float lastValidBattery = 4.0f;
unsigned long lastBatteryCheck = 0;
float currentBatteryVoltage = 4.0f;
bool batteryNeedsUpdate = true;
bool isCharging = false;

bool vibrationOn = false;
bool laserOn = false;

float gridItemScale[MAX_GRID_ITEMS];
float gridItemTargetScale[MAX_GRID_ITEMS];
unsigned long gridItemAnimStartTime[MAX_GRID_ITEMS];
bool gridAnimatingIn = false;
char marqueeText[40];
int marqueeTextLenPx = 0;
float marqueeOffset = 0;
unsigned long lastMarqueeTime = 0;
bool marqueeActive = false;
bool marqueePaused = false;
unsigned long marqueePauseStartTime = 0;
bool marqueeScrollLeft = true;
char SBUF[32];
bool btnPress0[8] = { false };
bool btnPress1[8] = { false };

// Wi-Fi Related Global Variable DEFINITIONS
String currentConnectedSsid = "";                             // Holds current connected SSID
KnownWifiNetwork knownNetworksList[MAX_KNOWN_WIFI_NETWORKS];  // List of known networks
int knownNetworksCount = 0;                                   // Number of known networks

WifiNetwork scannedNetworks[MAX_WIFI_NETWORKS];
int foundWifiNetworksCount = 0;
int wifiMenuIndex = 0;
bool wifiIsScanning = false;
unsigned long lastWifiScanCheckTime = 0;
float currentWifiListScrollOffset_Y_anim = 0.0f;
int targetWifiListScrollOffset_Y = 0;

char currentSsidToConnect[33];
char wifiPasswordInput[PASSWORD_MAX_LEN + 1];
int wifiPasswordInputCursor = 0;
bool selectedNetworkIsSecure = false;
bool wifiHardwareEnabled = false;  // Default to off

// Keyboard related globals
KeyboardLayer currentKeyboardLayer = KB_LAYER_LOWERCASE;
int keyboardFocusRow = 0;
int keyboardFocusCol = 0;
bool capsLockActive = false;

// Timer for connection attempt / info display
unsigned long wifiStatusMessageTimeout = 0;

// --- Wi-Fi Disconnect Overlay ---
// bool showPromptOverlay = false;
// int disconnectOverlaySelection = 0;

// --- Generic Prompt Overlay ---
bool showPromptOverlay = false;
int promptOverlaySelection = 0;  // 0 for "Cancel/No", 1 for "Confirm/Yes"
char promptOverlayTitle[32];
char promptOverlayMessage[64];                   // Potentially 2 lines
char promptOverlayOption0Text[10];               // e.g., "No", "Cancel"
char promptOverlayOption1Text[10];               // e.g., "Yes", "Confirm"
MenuState promptOverlayActionMenuTarget;         // Menu to go to if Option 1 is chosen
void (*promptOverlayConfirmAction)() = nullptr;  // Optional function to call on confirm

// --- Prompt Overlay Animation (can reuse disconnect animation vars) ---
float promptOverlayCurrentScale = 0.0f;
float promptOverlayTargetScale = 0.0f;
unsigned long promptOverlayAnimStartTime = 0;
bool promptOverlayAnimatingIn = false;

// --- Wi-Fi Disconnect Overlay Animation ---
// float disconnectOverlayCurrentScale = 0.0f;
// float disconnectOverlayTargetScale = 0.0f;
// unsigned long disconnectOverlayAnimStartTime = 0;
// bool disconnectOverlayAnimatingIn = false;

// --- Jamming Related Global Variable DEFINITIONS ---
bool isJammingOperationActive = false;
JammingType activeJammingType = JAM_NONE;
unsigned long lastJammingInputCheckTime = 0;  // Used for input polling in both modes
unsigned long currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_NORMAL;
unsigned long currentInputPollInterval = INPUT_POLL_INTERVAL_NORMAL;
int lastSelectedJammingToolGridIndex = 0;  // Definition for extern in config.h


// === Small Display Boot Log Globals ===
#define MAX_LOG_LINES_SMALL_DISPLAY 4
#define MAX_LOG_LINE_LENGTH_SMALL_DISPLAY 32
char smallDisplayLogBuffer[MAX_LOG_LINES_SMALL_DISPLAY][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY];

// === Boot Progress Globals ===
int totalBootSteps = 0;
int currentBootStep = 0;
float currentProgressBarFillPx_anim = 0.0f;
unsigned long segmentAnimStartTime = 0;
float segmentAnimDurationMs_float = 300.0f;
float segmentAnimStartPx = 0.0f;
float segmentAnimTargetPx = 0.0f;
const int PROGRESS_ANIM_UPDATE_INTERVAL_MS = 16;

// --- OTA Related Global Variable DEFINITIONS ---
int otaProgress = -2;
String otaStatusMessage = "";
bool otaIsUndoingChanges = false;
bool basicOtaStarted = false;
bool webOtaActive = false;
char otaDisplayIpAddress[40] = "";
FirmwareInfo currentRunningFirmware;  // <--- NEW DEFINITION


void updateMainDisplayBootProgress() {
  selectMux(MUX_CHANNEL_MAIN_DISPLAY);

  unsigned long currentTime = millis();
  float elapsedSegmentTime = 0;

  if (segmentAnimStartTime > 0 && segmentAnimDurationMs_float > 0) {
    elapsedSegmentTime = (float)(currentTime - segmentAnimStartTime);
  }

  float progress_t = 0.0f;
  if (segmentAnimDurationMs_float > 0) {
    progress_t = elapsedSegmentTime / segmentAnimDurationMs_float;
  }

  if (progress_t < 0.0f) progress_t = 0.0f;
  if (progress_t > 1.0f) progress_t = 1.0f;

  float eased_t = 1.0f - pow(1.0f - progress_t, 3);
  currentProgressBarFillPx_anim = segmentAnimStartPx + (segmentAnimTargetPx - segmentAnimStartPx) * eased_t;

  u8g2.firstPage();
  do {
    // ****** MODIFIED BITMAP RENDERING LOGIC ******
    // Ensure 'im_bits' is defined above. If it's not (e.g., only a comment placeholder),
    // this 'if' condition might behave unexpectedly or lead to compile errors
    // if 'im_bits' isn't declared at all.
    // The original check `sizeof(im_bits) > 1` assumes im_bits is a valid array.
    if (sizeof(im_bits) > 1) {  // Reverted to your simpler check
      u8g2.drawXBM(0, -2, im_width, im_height, im_bits);
    } else {
      u8g2.setFont(u8g2_font_ncenB10_tr);
      u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("KIVA")) / 2, 35, "KIVA");
    }
    // ****** END OF MODIFIED BITMAP LOGIC ******

    int progressBarY = im_height - 12 + 2;
    int progressBarHeight = 7;
    int progressBarWidth = u8g2.getDisplayWidth() - 40;  // Use display width
    int progressBarX = (u8g2.getDisplayWidth() - progressBarWidth) / 2;
    int progressBarDrawableWidth = progressBarWidth - 2;

    u8g2.drawRFrame(progressBarX, progressBarY, progressBarWidth, progressBarHeight, 1);

    int fillWidthToDraw = (int)round(currentProgressBarFillPx_anim);
    if (fillWidthToDraw > progressBarDrawableWidth) fillWidthToDraw = progressBarDrawableWidth;
    if (fillWidthToDraw < 0) fillWidthToDraw = 0;

    if (fillWidthToDraw > 0) {
      u8g2.drawRBox(progressBarX + 1, progressBarY + 1, fillWidthToDraw, progressBarHeight - 2, 0);
    }
  } while (u8g2.nextPage());
}


void logToSmallDisplay(const char* message, const char* status) {
  selectMux(MUX_CHANNEL_SECOND_DISPLAY);
  u8g2_small.setFont(u8g2_font_5x7_tf);
  char fullMessage[MAX_LOG_LINE_LENGTH_SMALL_DISPLAY];
  fullMessage[0] = '\0';
  int charsWritten = 0;
  if (status && strlen(status) > 0) {
    charsWritten = snprintf(fullMessage, sizeof(fullMessage), "[%s] ", status);
  }
  if (charsWritten < sizeof(fullMessage) - 1) {
    snprintf(fullMessage + charsWritten, sizeof(fullMessage) - charsWritten, "%.*s",
             (int)(sizeof(fullMessage) - charsWritten - 1), message);
  }
  fullMessage[MAX_LOG_LINE_LENGTH_SMALL_DISPLAY - 1] = '\0';
  for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY - 1; ++i) {
    strncpy(smallDisplayLogBuffer[i], smallDisplayLogBuffer[i + 1], MAX_LOG_LINE_LENGTH_SMALL_DISPLAY - 1);
    smallDisplayLogBuffer[i][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY - 1] = '\0';
  }
  strncpy(smallDisplayLogBuffer[MAX_LOG_LINES_SMALL_DISPLAY - 1], fullMessage, MAX_LOG_LINE_LENGTH_SMALL_DISPLAY - 1);
  smallDisplayLogBuffer[MAX_LOG_LINES_SMALL_DISPLAY - 1][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY - 1] = '\0';
  u8g2_small.clearBuffer();
  u8g2_small.setDrawColor(1);
  const int lineHeight = 8;
  int yPos = u8g2_small.getAscent();
  for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY; ++i) {
    u8g2_small.drawStr(2, yPos + (i * lineHeight), smallDisplayLogBuffer[i]);
  }
  u8g2_small.sendBuffer();
}


void setup() {
  for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY; ++i) {
    smallDisplayLogBuffer[i][0] = '\0';
  }
  Serial.begin(115200);
  Wire.begin();

  // keep this here, and display the placeholder boot log below
  // To turn off the laser and vibration motor as quick as possible when we boot.
  selectMux(0);
  writePCF(PCF0_ADDR, pcf0Output);

  // Initial RF mode
  currentRfMode = RF_MODE_OFF;  // Explicitly set initial RF mode

  selectMux(MUX_CHANNEL_SECOND_DISPLAY);
  u8g2_small.begin();
  u8g2_small.enableUTF8Print();
  u8g2_small.clearBuffer();
  u8g2_small.sendBuffer();

  selectMux(MUX_CHANNEL_MAIN_DISPLAY);
  u8g2.begin();
  u8g2.enableUTF8Print();

  totalBootSteps = 9;  // Adjusted for Jamming System
  currentBootStep = 0;
  currentProgressBarFillPx_anim = 0.0f;
  segmentAnimStartPx = 0.0f;
  segmentAnimTargetPx = 0.0f;
  segmentAnimStartTime = 0;
  const int stepDisplayDurationMs = 350;
  segmentAnimDurationMs_float = (float)stepDisplayDurationMs;

  updateMainDisplayBootProgress();
  delay(100);

  auto runBootStepAnimation = [&](const char* logMsg, const char* logStatus) {
    logToSmallDisplay(logMsg, logStatus);
    currentBootStep++;
    segmentAnimStartPx = currentProgressBarFillPx_anim;
    int progressBarTotalDrawableWidth = (u8g2.getDisplayWidth() - 40 - 2);  // Assuming im_width is u8g2.getDisplayWidth()
    segmentAnimTargetPx = (totalBootSteps > 0) ? (float)(progressBarTotalDrawableWidth * currentBootStep) / totalBootSteps : 0.0f;
    segmentAnimStartTime = millis();
    unsigned long currentSegmentLoopStartTime = millis();
    while (millis() - currentSegmentLoopStartTime < stepDisplayDurationMs) {
      updateMainDisplayBootProgress();
      delay(PROGRESS_ANIM_UPDATE_INTERVAL_MS);
    }
    currentProgressBarFillPx_anim = segmentAnimTargetPx;
    updateMainDisplayBootProgress();
  };

  runBootStepAnimation("Kiva Boot Agent v1.0", NULL);
  runBootStepAnimation("I2C Bus Initialized", "OK");

  runBootStepAnimation("PCF0 & MUX Setup", "OK");

  bool sdInitializedResult = setupSdCard();
  runBootStepAnimation("SD Card Mounted", sdInitializedResult ? "PASS" : "FAIL");

  if (sdInitializedResult) {
    loadCurrentFirmwareInfo();  // <--- NEW: Load current FW info after SD is up
  } else {
    // Set currentRunningFirmware to a default unknown state if SD fails
    currentRunningFirmware.isValid = false;
    strlcpy(currentRunningFirmware.version, "Unknown (No SD)", FW_VERSION_MAX_LEN);
    currentRunningFirmware.checksum_md5[0] = '\0';
  }

  analogReadResolution(12);
  setupBatteryMonitor();
  runBootStepAnimation("Battery Service", "OK");

  setupInputs();
  runBootStepAnimation("Input Handler", "OK");

  setupWifi();
  runBootStepAnimation("WiFi Subsystem", "INIT");

  setupJamming();
  runBootStepAnimation("Jamming System", "INIT");

  setupOtaManager();  // <--- NEW: Initialize OTA components
  runBootStepAnimation("OTA Manager", "INIT");

  runBootStepAnimation("Main Display OK", "READY");

  logToSmallDisplay("KivaOS Loading...", NULL);
  currentBootStep++;
  segmentAnimStartPx = currentProgressBarFillPx_anim;
  int progressBarTotalDrawableWidth = (u8g2.getDisplayWidth() - 40 - 2);
  segmentAnimTargetPx = (totalBootSteps > 0) ? (float)(progressBarTotalDrawableWidth * currentBootStep) / totalBootSteps : 0.0f;
  segmentAnimStartTime = millis();
  unsigned long finalStepLoopStartTime = millis();

  while (millis() - finalStepLoopStartTime < (stepDisplayDurationMs + 200)) {
    updateMainDisplayBootProgress();
    delay(PROGRESS_ANIM_UPDATE_INTERVAL_MS);
  }
  currentProgressBarFillPx_anim = segmentAnimTargetPx;
  updateMainDisplayBootProgress();
  delay(300);

  initializeCurrentMenu();
  wifiPasswordInput[0] = '\0';
}


void loop() {
  unsigned long currentTime = millis();

  // --- Priority 0: Critical Radio Mode Management ---
  // Example: If NRF jamming is active, ensure other RF tools are not.
  // This logic will be refined as tools are added.
  if (isJammingOperationActive && currentRfMode != RF_MODE_NRF_JAMMING) {
    // This indicates a potential state conflict. NRF jamming should take precedence.
    // For now, we assume NRF jamming setup correctly sets currentRfMode.
    // If other tools accidentally change currentRfMode without stopping NRF, log error.
    Serial.println("CRITICAL: NRF Jamming active but currentRfMode is not RF_MODE_NRF_JAMMING!");
  }
  if (!isJammingOperationActive && currentRfMode == RF_MODE_NRF_JAMMING) {
    // NRF jamming stopped, but mode not updated. Reset to RF_MODE_OFF.
    Serial.println("WARN: NRF Jamming stopped, resetting currentRfMode to RF_MODE_OFF.");
    currentRfMode = RF_MODE_OFF;
    // This implies stopActiveJamming() should also update currentRfMode.
  }

  // Priority 1: OTA Update processes (if active, they take precedence)
  if (currentMenu == OTA_WEB_ACTIVE) {
    if (webOtaActive) handleWebOtaClient();
    if (otaProgress == 100 && otaStatusMessage.indexOf("Rebooting") != -1) {
      drawUI();
      delay(2000);
      ESP.restart();
    }
  } else if (currentMenu == OTA_BASIC_ACTIVE) {
    if (basicOtaStarted) handleBasicOta();
    if (otaProgress == 100 && otaStatusMessage.indexOf("Rebooting") != -1) {
      drawUI();
      delay(2000);
      ESP.restart();
    }
  } else if (currentMenu == OTA_SD_STATUS) {
    if (otaProgress == OTA_PROGRESS_PENDING_SD_START) {
      performSdUpdate(g_firmwareToInstallFromSd);
    }
    if (otaProgress == 100 && otaStatusMessage.indexOf("Rebooting") != -1) {
      drawUI();
      delay(2000);
      ESP.restart();
    } else if (otaProgress == -3) {
      // UI will display "Already up to date". User can press BACK.
    } else if (otaProgress == -1) {
      // UI will display error. User can press BACK.
    }
  }

  if (isJammingOperationActive) {
    runJammerCycle();

    if (currentTime - lastJammingInputCheckTime > currentInputPollInterval) {
      updateInputs();

      if (btnPress1[NAV_BACK] || btnPress0[ENC_BTN]) {
        if (btnPress1[NAV_BACK]) btnPress1[NAV_BACK] = false;
        if (btnPress0[ENC_BTN]) btnPress0[ENC_BTN] = false;

        stopActiveJamming();
        currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_NORMAL;
        currentInputPollInterval = INPUT_POLL_INTERVAL_NORMAL;
        currentMenu = TOOL_CATEGORY_GRID;

        bool foundJammingCategory = false;
        for (int i = 0; i < getToolsMenuItemsCount(); ++i) {
          if (strcmp(toolsMenuItems[i], "Jamming") == 0) {
            toolsCategoryIndex = i;
            foundJammingCategory = true;
            break;
          }
        }
        if (!foundJammingCategory) {
          Serial.println("CRITICAL ERROR: 'Jamming' category not found in toolsMenuItems!");
          for (int i = 0; i < getToolsMenuItemsCount(); ++i) {  // Fallback
            if (strcmp(toolsMenuItems[i], "Jamming") == 0) {
              toolsCategoryIndex = i;
              break;
            }
          }
        }
        menuIndex = lastSelectedJammingToolGridIndex;
        int numJammingItems = getJammingToolItemsCount();
        if (numJammingItems > 0) {
          menuIndex = constrain(menuIndex, 0, numJammingItems - 1);
        } else {
          menuIndex = 0;
        }
        initializeCurrentMenu();
        drawUI();  // Draw immediately after stopping jamming
      }
      lastJammingInputCheckTime = currentTime;
    }
  } else {  // Not Jamming
    if (currentTime - lastJammingInputCheckTime > currentInputPollInterval) {
      updateInputs();
      lastJammingInputCheckTime = currentTime;
    }

    if (batteryNeedsUpdate || (currentTime - lastBatteryCheck >= currentBatteryCheckInterval)) {
      getSmoothV();
    }

    // Revised Wi-Fi auto-disable logic considering currentRfMode
    if (wifiHardwareEnabled && WiFi.status() != WL_CONNECTED && currentRfMode == RF_MODE_NORMAL_STA) {
      bool stayOnForMenu = (currentMenu == WIFI_SETUP_MENU || currentMenu == WIFI_PASSWORD_INPUT || currentMenu == WIFI_CONNECTING || currentMenu == WIFI_CONNECTION_INFO || showPromptOverlay);

      bool stayOnForOtaError = (currentMenu == OTA_BASIC_ACTIVE && otaProgress == -1 && (otaStatusMessage.indexOf("Connect to Wi-Fi") != -1 || otaStatusMessage.indexOf("Failed to enable Wi-Fi") != -1));
      bool stayOnForOtaReady = (currentMenu == OTA_BASIC_ACTIVE && basicOtaStarted && otaProgress == 0);
      bool stayOnForWebOta = webOtaActive;

      WifiConnectionStatus currentManagerStatus = getCurrentWifiConnectionStatus();
      bool stayOnForActiveConnectionAttempt = (wifiIsScanning || currentManagerStatus == WIFI_CONNECTING_IN_PROGRESS || currentManagerStatus == KIVA_WIFI_SCAN_RUNNING);

      if (!stayOnForMenu && !stayOnForActiveConnectionAttempt && !stayOnForOtaReady && !stayOnForOtaError && !stayOnForWebOta) {
        Serial.println("Loop: Wi-Fi ON (STA), Not Connected, Not in critical Menu/State. Disabling Wi-Fi.");
        setWifiHardwareState(false, RF_MODE_OFF);  // Explicitly set mode to OFF
      }
    }

    // Wi-Fi Scan & Connection Progress
    if (currentMenu == WIFI_SETUP_MENU && !showPromptOverlay) {
      if (wifiIsScanning) {                       // Kiva's state: we believe a scan should be in progress
        int espScanStatus = WiFi.scanComplete();  // Query ESP32's actual scan state

        if (espScanStatus == WIFI_SCAN_RUNNING) {  // ESP32 confirms scan is running
          if (currentTime - lastWifiScanCheckTime >= WIFI_SCAN_CHECK_INTERVAL) {
            int result = checkAndRetrieveWifiScanResults();
            if (!wifiIsScanning) {  // Scan just finished
              initializeCurrentMenu();
              wifiMenuIndex = 0;
              targetWifiListScrollOffset_Y = 0;
              currentWifiListScrollOffset_Y_anim = 0;
              if (maxMenuItems > 0) wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
            }
            lastWifiScanCheckTime = currentTime;
          }
        } else {                              // ESP32 is NOT running a scan, but Kiva's wifiIsScanning flag is true.
          checkAndRetrieveWifiScanResults();  // This will update wifiIsScanning to false if scan is done/failed.
          if (!wifiIsScanning) {              // If it's truly finished/failed
            initializeCurrentMenu();
            wifiMenuIndex = 0;
            targetWifiListScrollOffset_Y = 0;
            currentWifiListScrollOffset_Y_anim = 0;
            if (maxMenuItems > 0) wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
          }
        }
      }
    } else if (currentMenu == WIFI_CONNECTING && !showPromptOverlay) {
      WifiConnectionStatus status = checkWifiConnectionProgress();
      if (status != WIFI_CONNECTING_IN_PROGRESS) {
        currentMenu = WIFI_CONNECTION_INFO;
        wifiStatusMessageTimeout = millis() + 3000;
        initializeCurrentMenu();
      }
    } else if (currentMenu == WIFI_CONNECTION_INFO && !showPromptOverlay) {
      if (millis() > wifiStatusMessageTimeout) {
        WifiConnectionStatus lastStatus = getCurrentWifiConnectionStatus();
        if (lastStatus == WIFI_CONNECTED_SUCCESS) {
          if (wifiConnectForScheduledAction) {
            currentMenu = postWifiActionMenu;
            wifiConnectForScheduledAction = false;
            if (currentMenu == OTA_BASIC_ACTIVE) {
              if (!startBasicOtaUpdate()) {
                // Error starting OTA, OTA_BASIC_ACTIVE screen will show new error
              }
            }
          } else {
            currentMenu = WIFI_SETUP_MENU;
          }
        } else if (selectedNetworkIsSecure && (lastStatus == WIFI_FAILED_WRONG_PASSWORD || lastStatus == WIFI_FAILED_TIMEOUT)) {
          currentMenu = WIFI_SETUP_MENU;  // Or WIFI_PASSWORD_INPUT for retry
        } else {
          currentMenu = WIFI_SETUP_MENU;
        }
        initializeCurrentMenu();
      }
    }

    bool criticalOtaUpdateInProgress = false;
    if (currentMenu == OTA_WEB_ACTIVE || currentMenu == OTA_SD_STATUS || currentMenu == OTA_BASIC_ACTIVE) {
      if ((otaProgress > 0 && otaProgress < 100) || (otaProgress == 100 && otaStatusMessage.indexOf("Rebooting") != -1)) {
        criticalOtaUpdateInProgress = true;
      }
    }

    bool allowMenuInput = !criticalOtaUpdateInProgress;
    if (showPromptOverlay) {
      allowMenuInput = false;
    }

    if (allowMenuInput) {
      bool selectionKeyPressed = false;
      if (btnPress1[NAV_OK]) {
        btnPress1[NAV_OK] = false;
        selectionKeyPressed = true;
      }
      if (btnPress0[ENC_BTN]) {
        if (currentMenu != WIFI_PASSWORD_INPUT) {
          btnPress0[ENC_BTN] = false;
          selectionKeyPressed = true;
        }
      }

      if (selectionKeyPressed) {
        handleMenuSelection();
      }

      if (btnPress1[NAV_BACK]) {
        btnPress1[NAV_BACK] = false;
        handleMenuBackNavigation();
      }
    }

    if (allowMenuInput && !showPromptOverlay) {
      // Clear other one-shot PCF0 buttons if not consumed by specific actions
      // Example: if (btnPress0[BTN_AI]) btnPress0[BTN_AI] = false;
    }

    if (!showPromptOverlay && currentMenu != WIFI_PASSWORD_INPUT && allowMenuInput) {
      btnPress1[NAV_UP] = false;
      btnPress1[NAV_DOWN] = false;
      btnPress1[NAV_LEFT] = false;
      btnPress1[NAV_RIGHT] = false;
    }

    // --- Prompt Overlay Confirmation Side Effects ---
    // This section handles actions *after* a prompt has been confirmed (or cancelled)
    // and `showPromptOverlay` has just become false in the current loop iteration.
    // `updateInputs()` handles the direct input and sets `showPromptOverlay = false`.
    // `handleMenuSelection()` or `handleMenuBackNavigation()` might have been called by `updateInputs()`
    // or by the main loop's logic based on `btnPress` flags from the prompt.
    // If a menu change was intended by the prompt, `initializeCurrentMenu()` will be called.
    // This specific block is for actions tied to the *completion* of a prompt interaction
    // that leads to the WIFI_SETUP_MENU for a scheduled action.
    static bool wasPromptActiveLastLoop = false;

    if (wasPromptActiveLastLoop && !showPromptOverlay) {  // Prompt just got dismissed
      if (currentMenu == WIFI_SETUP_MENU && wifiConnectForScheduledAction) {
        Serial.println("Loop: Prompt dismissed, now in WIFI_SETUP_MENU for scheduled action.");

        if (!wifiHardwareEnabled) {
          Serial.println("Loop: Wi-Fi was unexpectedly OFF. Enabling for scheduled action scan...");
          // The error is likely here or in similar calls:
          // OLD: setWifiHardwareState(true, WIFI_MODE_STA);
          setWifiHardwareState(true, RF_MODE_NORMAL_STA); // <--- UPDATED: Use ActiveRfOperationMode
          delay(300);
        } else {
          delay(100);
        }

        if (wifiHardwareEnabled) {
          wifiMenuIndex = 0;
          targetWifiListScrollOffset_Y = 0;
          currentWifiListScrollOffset_Y_anim = 0;
          // Set Kiva's state to "intending to scan" *before* calling initializeCurrentMenu
          wifiIsScanning = true;
          foundWifiNetworksCount = 0;

          initializeCurrentMenu();  // Update UI to "Scanning..." state based on wifiIsScanning = true

          Serial.println("Loop: Attempting to initiate scan from prompt completion logic.");
          int scanReturn = initiateAsyncWifiScan();  // This sets internal wifiStatusString in wifi_manager

          if (scanReturn == WIFI_SCAN_FAILED || (scanReturn != WIFI_SCAN_RUNNING && scanReturn != -1)) {
            Serial.println("Loop: Initial scan attempt failed. Retrying once...");
            delay(250);  // Slightly longer delay for retry
            scanReturn = initiateAsyncWifiScan();
            if (scanReturn == WIFI_SCAN_FAILED || (scanReturn != WIFI_SCAN_RUNNING && scanReturn != -1)) {
              Serial.println("Loop: Scan retry also failed.");
              wifiIsScanning = false;
              checkAndRetrieveWifiScanResults();  // Updates wifiStatusString to "Scan Failed"
              initializeCurrentMenu();            // Re-init to show "Scan Failed" state properly
            } else if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) {
              Serial.println("Loop: Scan retry initiated successfully (async).");
              lastWifiScanCheckTime = millis();
              // wifiIsScanning is already true, initializeCurrentMenu was called.
            } else {
              Serial.println("Loop: Scan retry succeeded synchronously.");
              wifiIsScanning = false;
              checkAndRetrieveWifiScanResults();
              initializeCurrentMenu();
            }
          } else if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) {
            Serial.println("Loop: Scan initiated successfully (async).");
            lastWifiScanCheckTime = millis();
            // wifiIsScanning is true, initializeCurrentMenu was called.
          } else {
            Serial.println("Loop: Scan succeeded synchronously on first try.");
            wifiIsScanning = false;
            checkAndRetrieveWifiScanResults();
            initializeCurrentMenu();
          }
        } else {
          Serial.println("Loop: Wi-Fi hardware failed to enable after prompt. Cancelling scheduled action.");
          wifiConnectForScheduledAction = false;
          wifiIsScanning = false;  // Ensure this is false
          initializeCurrentMenu();
        }
      }
      // Clear wifiConnectForScheduledAction if the action was for WIFI_SETUP_MENU and it's now handled.
      // However, it's better to clear it when the *actual* scheduled action (e.g. OTA) is completed or aborted.
      // For now, this specific logic only handles getting to WIFI_SETUP_MENU for the scan.
    }
    wasPromptActiveLastLoop = showPromptOverlay;

    drawUI();
  }  // End of "else" for "if (isJammingOperationActive)"

  // Conditional delay AT THE END OF THE LOOP
  if (isJammingOperationActive) {
    delay(JAMMER_CYCLE_DELAY_MS);
  } else if (webOtaActive) {
    if (otaProgress > 0 && otaProgress < 100) {
      delay(0);
    } else {
      delay(1);
    }
  } else if (basicOtaStarted) {
    delay(1);
  } else {
    delay(16);
  }
}