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
#include "jamming.h" // <--- NEW INCLUDE

// ... (Bitmap Data) ...
#define im_width 128
#define im_height 64

static const unsigned char im_bits[] U8X8_PROGMEM = { // Added U8X8_PROGMEM to save RAM
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0xe0,0x03,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0xe0,0x0f,0x00,0x00,0x00,0x00,0xc0,0x1f,0x00,0x00,0x00,0x20,
0x00,0x00,0x00,0x00,0xe2,0x1f,0x00,0x00,0x00,0x00,0xf0,0x0f,0x18,0x00,0x00,
0x38,0x00,0x00,0x00,0x80,0xf1,0x1f,0x00,0x00,0x00,0x00,0xf8,0x07,0x1c,0x00,
0x00,0x3f,0x00,0x00,0x00,0xe0,0xf0,0x2f,0x00,0x00,0x00,0x00,0xfc,0x07,0x1e,
0x00,0x80,0x3f,0x00,0x00,0x00,0xf0,0xf0,0x27,0x00,0x00,0x00,0x00,0xfe,0x03,
0x1f,0x00,0x80,0x3f,0x00,0x00,0x00,0x7c,0xf0,0x47,0x00,0x00,0x00,0x00,0xff,
0x81,0x3f,0x00,0x80,0x3f,0x00,0x00,0x00,0x7f,0xf0,0xc7,0x00,0x00,0x00,0x80,
0xff,0xc0,0x3f,0x00,0x80,0x1f,0x00,0x00,0x80,0x3f,0xf8,0xc7,0x00,0x00,0x00,
0xc0,0x7f,0xe0,0x3f,0x00,0x80,0x1f,0x00,0x00,0xe0,0x3f,0xf8,0x83,0x01,0x00,
0x00,0xe0,0x3f,0xf0,0x7f,0x00,0xc0,0x1f,0x00,0x00,0xf8,0x1f,0xf8,0x83,0x03,
0x00,0x00,0xe0,0x1f,0xf8,0x7f,0x00,0xc0,0x1f,0x00,0x00,0xfe,0x1f,0xf8,0x03,
0x03,0x00,0x00,0xf0,0x0f,0xfc,0x7f,0x00,0xc0,0x1f,0x00,0x00,0xff,0x0f,0xf8,
0x03,0x07,0x00,0x00,0xf8,0x07,0xfe,0xfd,0x00,0xc0,0x1f,0x00,0xc0,0xff,0x07,
0xfc,0x03,0x0f,0x00,0x00,0xfc,0x03,0xff,0xfc,0x00,0xc0,0x1f,0x00,0xf0,0xff,
0x01,0xfc,0x01,0x0e,0x00,0x00,0xfe,0x83,0x7f,0xfc,0x00,0xe0,0x0f,0x00,0xfc,
0x7f,0x00,0xfc,0x01,0x1e,0x00,0x00,0xff,0xc1,0x3f,0xf8,0x01,0xe0,0x0f,0x00,
0xfe,0x1f,0x00,0xfc,0x01,0x3c,0x00,0x80,0xff,0xf0,0x1f,0xf8,0x01,0xe0,0x0f,
0x80,0xff,0x07,0x00,0xfc,0x01,0x3c,0x00,0xc0,0x7f,0xf8,0x0f,0xf8,0x01,0xe0,
0x0f,0xe0,0xff,0x01,0x00,0xfe,0x00,0x7c,0x00,0xe0,0x3f,0xfc,0x07,0xf8,0x03,
0xf0,0x0f,0xf0,0x7f,0x00,0x00,0xfe,0x00,0xf8,0x00,0xf0,0x1f,0xfe,0x03,0xf0,
0x03,0xf0,0x0f,0xfc,0x1f,0x00,0x00,0xfe,0x00,0xf8,0x00,0xf8,0x0f,0xff,0x01,
0xf0,0x03,0xf0,0x0f,0xff,0x07,0x00,0x00,0xfe,0x00,0xf8,0x01,0xfc,0x87,0xff,
0x00,0xf0,0x07,0xf0,0xcf,0xff,0x01,0x00,0x00,0xfe,0x00,0xf0,0x03,0xfe,0xc3,
0x7f,0x00,0xf0,0x07,0xf0,0xe7,0x7f,0x00,0x00,0x00,0x7f,0x00,0xf0,0x03,0xff,
0xe1,0x1f,0x00,0xe0,0x07,0xf8,0xff,0xff,0x01,0x00,0x00,0x7f,0x00,0xe0,0x87,
0xff,0xf1,0x0f,0x00,0xe0,0x07,0xf8,0xe7,0xff,0x07,0x00,0x00,0x7f,0x00,0xe0,
0xcf,0xff,0xf8,0x07,0x00,0xe0,0x0f,0xf8,0x07,0xff,0x0f,0x00,0x00,0x7f,0x00,
0xe0,0xef,0x7f,0xfc,0x03,0x00,0xe0,0x0f,0xf8,0x07,0xfc,0x3f,0x00,0x00,0x3f,
0x00,0xc0,0xff,0x3f,0xfe,0x01,0x00,0xc0,0x0f,0xf8,0x07,0xe0,0xff,0x00,0x80,
0x3f,0x00,0xc0,0xff,0x1f,0xff,0x00,0x00,0xfc,0x1f,0xfc,0x07,0x00,0xff,0x01,
0x80,0x3f,0x00,0x80,0xff,0xcf,0x7f,0xfe,0xff,0xc7,0x1f,0xfc,0x07,0x00,0xfc,
0x07,0x80,0x3f,0x00,0x80,0xff,0xe7,0xff,0xff,0xff,0xc0,0x1f,0xfc,0x03,0x00,
0xe0,0x1f,0x00,0x3f,0x00,0x80,0xff,0xe3,0xff,0xff,0x0f,0x80,0x3f,0xfe,0x03,
0x00,0x00,0x7f,0x00,0x1f,0x00,0x00,0xff,0xc1,0xff,0xff,0x01,0x80,0x3f,0xfe,
0x03,0x00,0x00,0xf8,0x00,0x1f,0x00,0x00,0xff,0x80,0xff,0x3f,0x00,0x80,0x3f,
0xfc,0x03,0x00,0x00,0xe0,0x03,0x1e,0x00,0x00,0xff,0x00,0xff,0x07,0x00,0x80,
0x3f,0xf8,0x03,0x00,0x00,0x00,0x0f,0x1e,0x00,0x00,0x7e,0x00,0xfe,0x00,0x00,
0x00,0x3f,0xe0,0x03,0x00,0x00,0x00,0x38,0x0e,0x00,0x00,0x3e,0x00,0x0c,0x00,
0x00,0x00,0x1f,0xc0,0x03,0x00,0x00,0x00,0xe0,0x0c,0x00,0x00,0x1c,0x00,0x00,
0x00,0x00,0x00,0x1f,0x00,0x01,0x00,0x00,0x00,0x00,0x0f,0x00,0x00,0x0c,0x00,
0x00,0x00,0x00,0x00,0x0f,0x00,0x02,0x00,0x00,0x00,0x00,0x0c,0x00,0x00,0x04,
0x00,0x00,0x00,0x00,0x00,0x0e,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00
};

// === Global Object Definitions ===
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2_small(U8G2_R0, U8X8_PIN_NONE);
QuadratureEncoder encoder; // Definition
VerticalListAnimation mainMenuAnim;
CarouselAnimation subMenuAnim;
VerticalListAnimation wifiListAnim;

// === Global Variable Definitions ===
MenuState currentMenu = MAIN_MENU;
int menuIndex = 0;
int maxMenuItems = 0;
int mainMenuSavedIndex = 0;
int toolsCategoryIndex = 0;
int gridCols = 2;
int targetGridScrollOffset_Y = 0;
float currentGridScrollOffset_Y_anim = 0.0f;
uint8_t pcf0Output = 0xFF & ~((1 << LASER_PIN_PCF0) | (1 << MOTOR_PIN_PCF0)); // Ensure LASER and MOTOR pins are off initially

float batReadings[BAT_SAMPLES] = {0};
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
bool btnPress0[8] = {false};
bool btnPress1[8] = {false};

// Wi-Fi Related Global Variable DEFINITIONS
String currentConnectedSsid = ""; // Holds current connected SSID
KnownWifiNetwork knownNetworksList[MAX_KNOWN_WIFI_NETWORKS]; // List of known networks
int knownNetworksCount = 0; // Number of known networks

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
bool wifiHardwareEnabled = false; // Default to off

// Keyboard related globals
KeyboardLayer currentKeyboardLayer = KB_LAYER_LOWERCASE;
int keyboardFocusRow = 0;
int keyboardFocusCol = 0;
bool capsLockActive = false;

// Timer for connection attempt / info display
unsigned long wifiStatusMessageTimeout = 0;

// --- Wi-Fi Disconnect Overlay ---
bool showWifiDisconnectOverlay = false;
int disconnectOverlaySelection = 0;

// --- Wi-Fi Disconnect Overlay Animation ---
float disconnectOverlayCurrentScale = 0.0f;
float disconnectOverlayTargetScale = 0.0f;
unsigned long disconnectOverlayAnimStartTime = 0;
bool disconnectOverlayAnimatingIn = false;

// --- Jamming Related Global Variable DEFINITIONS ---
bool isJammingOperationActive = false;
JammingType activeJammingType = JAM_NONE;
unsigned long lastJammingInputCheckTime = 0; // Used for input polling in both modes
unsigned long currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_NORMAL;
unsigned long currentInputPollInterval = INPUT_POLL_INTERVAL_NORMAL;
int lastSelectedJammingToolGridIndex = 0; // Definition for extern in config.h


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
        if (sizeof(im_bits) > 1) { // Reverted to your simpler check
            u8g2.drawXBM(0, -2, im_width, im_height, im_bits);
        } else {
            u8g2.setFont(u8g2_font_ncenB10_tr);
            u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("KIVA")) / 2, 35, "KIVA");
        }
        // ****** END OF MODIFIED BITMAP LOGIC ******

        int progressBarY = im_height - 12 + 2;
        int progressBarHeight = 7;
        int progressBarWidth = u8g2.getDisplayWidth() - 40; // Use display width
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
    strncpy(smallDisplayLogBuffer[i], smallDisplayLogBuffer[i+1], MAX_LOG_LINE_LENGTH_SMALL_DISPLAY -1);
    smallDisplayLogBuffer[i][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY - 1] = '\0';
  }
  strncpy(smallDisplayLogBuffer[MAX_LOG_LINES_SMALL_DISPLAY - 1], fullMessage, MAX_LOG_LINE_LENGTH_SMALL_DISPLAY -1);
  smallDisplayLogBuffer[MAX_LOG_LINES_SMALL_DISPLAY - 1][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY -1] = '\0';
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

  selectMux(MUX_CHANNEL_SECOND_DISPLAY);
  u8g2_small.begin();
  u8g2_small.enableUTF8Print();
  u8g2_small.clearBuffer();
  u8g2_small.sendBuffer();

  selectMux(MUX_CHANNEL_MAIN_DISPLAY);
  u8g2.begin();
  u8g2.enableUTF8Print();

  totalBootSteps = 9; // Adjusted for Jamming System
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
    int progressBarTotalDrawableWidth = (u8g2.getDisplayWidth() - 40 - 2); // Assuming im_width is u8g2.getDisplayWidth()
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

  analogReadResolution(12);
  setupBatteryMonitor();
  runBootStepAnimation("Battery Service", "OK");

  setupInputs();
  runBootStepAnimation("Input Handler", "OK");

  setupWifi();
  runBootStepAnimation("WiFi Subsystem", "INIT");

  setupJamming();
  runBootStepAnimation("Jamming System", "INIT"); // New boot step
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

  if (isJammingOperationActive) {
    runJammerCycle(); // Run the NRF modules for one jamming cycle

    if (currentTime - lastJammingInputCheckTime > currentInputPollInterval) {
      updateInputs(); // Poll inputs less frequently

      // Check for NAV_BACK or ENC_BTN to stop jamming
      if (btnPress1[NAV_BACK] || btnPress0[ENC_BTN]) {
        if (btnPress1[NAV_BACK]) btnPress1[NAV_BACK] = false;
        if (btnPress0[ENC_BTN]) btnPress0[ENC_BTN] = false;

        stopActiveJamming(); // This sets isJammingOperationActive = false, activeJammingType = JAM_NONE

        // Restore normal operation speeds
        currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_NORMAL;
        currentInputPollInterval = INPUT_POLL_INTERVAL_NORMAL;

        // Return to the jamming selection grid in Tools menu
        currentMenu = TOOL_CATEGORY_GRID;

        // Ensure toolsCategoryIndex is correctly set to "Jamming"
        bool foundJammingCategory = false;
        for(int i=0; i < getToolsMenuItemsCount(); ++i) {
            if(strcmp(toolsMenuItems[i], "Jamming") == 0) {
                toolsCategoryIndex = i;
                foundJammingCategory = true;
                break;
            }
        }
        if (!foundJammingCategory) {
            // This should not happen if menu items are consistent. Fallback or error.
            Serial.println("CRITICAL ERROR: 'Jamming' category not found in toolsMenuItems!");
            // Default to a known index for Jamming if programmatic find fails (e.g. 4 based on comments)
            // This is a safeguard; ideally, the string comparison should always work.
             for(int i=0; i < getToolsMenuItemsCount(); ++i) { // Find the default index for "Jamming"
                if(strcmp(toolsMenuItems[i], "Jamming") == 0) { toolsCategoryIndex = i; break;}
            }
        }

        menuIndex = lastSelectedJammingToolGridIndex; // Restore the last selected jamming tool

        // Validate menuIndex against the items in the Jamming category
        int numJammingItems = getJammingToolItemsCount();
        if (numJammingItems > 0) {
            menuIndex = constrain(menuIndex, 0, numJammingItems - 1);
        } else {
            menuIndex = 0; // No items, index must be 0
        }

        initializeCurrentMenu(); // Re-initialize menu state for drawing
        drawUI(); // Draw the UI once to reflect the change
      }
      lastJammingInputCheckTime = currentTime;
    }
    // No regular UI drawing or other updates while actively jamming to maximize NRF performance
  } else {
    // --- Normal Operation Loop ---
    if (currentTime - lastJammingInputCheckTime > currentInputPollInterval) {
        updateInputs();
        lastJammingInputCheckTime = currentTime;
    }

    // Battery update logic
    if (batteryNeedsUpdate || (currentTime - lastBatteryCheck >= currentBatteryCheckInterval)) {
        getSmoothV();
    }

    // Wi-Fi auto-disable logic (from your provided KivaMain.ino)
    if (wifiHardwareEnabled && WiFi.status() != WL_CONNECTED) {
        bool inRelevantWifiMenu = (currentMenu == WIFI_SETUP_MENU ||
                                   currentMenu == WIFI_PASSWORD_INPUT ||
                                   currentMenu == WIFI_CONNECTING ||
                                   currentMenu == WIFI_CONNECTION_INFO ||
                                   showWifiDisconnectOverlay);

        WifiConnectionStatus currentManagerStatus = getCurrentWifiConnectionStatus();

        bool isActivelyTrying = (wifiIsScanning ||
                                 currentManagerStatus == WIFI_CONNECTING_IN_PROGRESS ||
                                 currentManagerStatus == KIVA_WIFI_SCAN_RUNNING);

        if (!inRelevantWifiMenu && !isActivelyTrying) {
            Serial.println("Loop: Wi-Fi ON, Not Connected, Not in Wi-Fi Menu/Overlay, Not Actively Trying. Disabling Wi-Fi.");
            setWifiHardwareState(false); // This will also update UI/menu state if needed
        }
    }

    // Wi-Fi scan check (modified for clarity and to rely on checkAndRetrieveWifiScanResults)
    if (currentMenu == WIFI_SETUP_MENU && wifiIsScanning && !showWifiDisconnectOverlay) {
      if (currentTime - lastWifiScanCheckTime >= WIFI_SCAN_CHECK_INTERVAL) {
        int result = checkAndRetrieveWifiScanResults(); // This function now handles WiFi.scanComplete()

        // wifiIsScanning is set to false by checkAndRetrieveWifiScanResults if scan is done/failed.
        if (!wifiIsScanning) {
          initializeCurrentMenu(); // Re-initialize menu for new item count, title, etc.
          // Reset scroll and animation for the list as it's re-populated
          wifiMenuIndex = 0;
          targetWifiListScrollOffset_Y = 0;
          currentWifiListScrollOffset_Y_anim = 0;
          if (maxMenuItems > 0) wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems); // Update animation targets
        }
        lastWifiScanCheckTime = currentTime; // Update check time
      }
    } else if (currentMenu == WIFI_CONNECTING && !showWifiDisconnectOverlay) {
        WifiConnectionStatus status = checkWifiConnectionProgress();
        if (status != WIFI_CONNECTING_IN_PROGRESS) {
            currentMenu = WIFI_CONNECTION_INFO;
            wifiStatusMessageTimeout = millis() + 3000; // Show result for 3 seconds
            initializeCurrentMenu();
        }
    } else if (currentMenu == WIFI_CONNECTION_INFO && !showWifiDisconnectOverlay) {
        if (millis() > wifiStatusMessageTimeout) {
            WifiConnectionStatus lastStatus = getCurrentWifiConnectionStatus();
            if (lastStatus == WIFI_CONNECTED_SUCCESS) {
                currentMenu = WIFI_SETUP_MENU; // Go back to Wi-Fi list
            } else if (selectedNetworkIsSecure && (lastStatus == WIFI_FAILED_WRONG_PASSWORD || lastStatus == WIFI_FAILED_TIMEOUT)) {
                // If known password failed multiple times, go to password input
                KnownWifiNetwork* net = findKnownNetwork(currentSsidToConnect);
                if (net && net->failCount >= MAX_WIFI_FAIL_ATTEMPTS) {
                    currentMenu = WIFI_PASSWORD_INPUT;
                } else {
                    currentMenu = WIFI_SETUP_MENU; // Otherwise, back to list
                }
            } else { // Other failures, or open network failure
                currentMenu = WIFI_SETUP_MENU;
            }
            initializeCurrentMenu();
        }
    }

    // Menu Navigation and Selection (from your provided KivaMain.ino)
    if (!showWifiDisconnectOverlay) {
      if (btnPress1[NAV_OK] || btnPress0[ENC_BTN]) {
        if (btnPress1[NAV_OK]) btnPress1[NAV_OK] = false;
        if (btnPress0[ENC_BTN]) btnPress0[ENC_BTN] = false;
        if (currentMenu != WIFI_PASSWORD_INPUT) { // Keyboard OK is handled in updateInputs
            handleMenuSelection();
        }
      }

      if (btnPress1[NAV_BACK]) {
        btnPress1[NAV_BACK] = false;
        handleMenuBackNavigation();
      }
    }

    // Clear button press flags (from your provided KivaMain.ino, slightly adapted)
    for (int i = 0; i < 8; ++i) {
      // ENC_BTN (pcf0[0]) is handled specially for keyboard and overlay confirmation.
      // It's consumed in updateInputs for keyboard, and in main loop for overlay.
      // If it reaches here and wasn't consumed, it means it's for a general menu OK.
      // General menu OK (ENC_BTN) is cleared when handleMenuSelection is called (or if not used).
      // So, explicit clearing of btnPress0[ENC_BTN] here might be redundant or could interfere
      // if it's meant to be processed by handleMenuSelection.
      // Let's keep your original logic for now, but it's a point of attention.
      if ((currentMenu == WIFI_PASSWORD_INPUT || showWifiDisconnectOverlay) && i == ENC_BTN) {
          // Don't clear ENC_BTN if it's for keyboard or overlay, it's handled there
      } else {
          btnPress0[i] = false; // Clear other PCF0 buttons
      }
    }

    if (!showWifiDisconnectOverlay && currentMenu != WIFI_PASSWORD_INPUT) {
      // These directional flags are for one-shot presses.
      // If scrollAct consumes them, fine. If not, they are cleared here.
      // Auto-repeat logic in updateInputs uses direct pin state, not these flags.
      btnPress1[NAV_UP] = false;
      btnPress1[NAV_DOWN] = false;
      btnPress1[NAV_LEFT] = false;
      btnPress1[NAV_RIGHT] = false;
    }
    // NAV_OK and NAV_BACK (btnPress1[NAV_OK] and btnPress1[NAV_BACK]) are cleared
    // where they are handled (menu selection/back, overlay confirmation/cancel).

    drawUI(); // Draw the full UI in normal operation
  }

  // Conditional delay
  if (!isJammingOperationActive) {
    delay(16); // Approx 60 FPS for UI
  } else {
    delay(JAMMER_CYCLE_DELAY_MS); // Small delay to allow some system tasks if needed during jamming
  }
}