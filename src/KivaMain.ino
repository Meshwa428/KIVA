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
#include "firmware_metadata.h" // <--- NEW INCLUDE

// For OTA Web Server and Basic OTA
#include <WebServer.h>   // Already in config.h for extern WebServer
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Update.h>      // For Update class, used by ota_manager

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
WebServer otaWebServer(OTA_WEB_SERVER_PORT); // Unchanged

FirmwareInfo g_firmwareToInstallFromSd;

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

// --- OTA Related Global Variable DEFINITIONS ---
int otaProgress = -2; 
String otaStatusMessage = "";
bool otaIsUndoingChanges = false;
bool basicOtaStarted = false;
bool webOtaActive = false;
char otaDisplayIpAddress[40] = "";
FirmwareInfo currentRunningFirmware; // <--- NEW DEFINITION


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

  if (sdInitializedResult) {
    loadCurrentFirmwareInfo(); // <--- NEW: Load current FW info after SD is up
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

  setupOtaManager(); // <--- NEW: Initialize OTA components
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

  // Priority 1: OTA Update processes (if active, they take precedence)
  if (currentMenu == OTA_WEB_ACTIVE) {
    if (webOtaActive) handleWebOtaClient();
    if (otaProgress == 100 && otaStatusMessage.indexOf("Rebooting") != -1) {
      drawUI(); delay(2000); ESP.restart();
    }
  } else if (currentMenu == OTA_BASIC_ACTIVE) {
    if (basicOtaStarted) handleBasicOta();
    if (otaProgress == 100 && otaStatusMessage.indexOf("Rebooting") != -1) {
      drawUI(); delay(2000); ESP.restart();
    }
  } else if (currentMenu == OTA_SD_STATUS) {
    // Check if we are in the "pending start" phase for SD update
    if (otaProgress == OTA_PROGRESS_PENDING_SD_START) {
      // The "Preparing..." message has been shown by drawUI() in the previous loop iteration.
      // Now, perform the actual update.
      performSdUpdate(g_firmwareToInstallFromSd); // This will update otaProgress and otaStatusMessage further
      // performSdUpdate sets otaProgress to other values (e.g., 5, 10, -1, 100, -3)
    }
    
    // Standard check for reboot after completion (or handling "already up-to-date")
    if (otaProgress == 100 && otaStatusMessage.indexOf("Rebooting") != -1) {
      drawUI(); delay(2000); ESP.restart();
    } else if (otaProgress == -3) { // "Already up to date"
      // UI will display this message. User can press BACK.
    } else if (otaProgress == -1) { // Error
      // UI will display this message. User can press BACK.
    }
  }

  if (isJammingOperationActive) {
    runJammerCycle(); // Run the NRF modules for one jamming cycle

    if (currentTime - lastJammingInputCheckTime > currentInputPollInterval) {
      updateInputs(); // Poll inputs less frequently

      // Check for NAV_BACK or ENC_BTN to stop jamming
      if (btnPress1[NAV_BACK] || btnPress0[ENC_BTN]) {
        // Consume flags immediately
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
          // Fallback to find "Jamming" again, just in case.
          for (int i = 0; i < getToolsMenuItemsCount(); ++i) {
            if (strcmp(toolsMenuItems[i], "Jamming") == 0) { toolsCategoryIndex = i; break; }
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
        drawUI();
      }
      lastJammingInputCheckTime = currentTime;
    }
    // No regular UI drawing or other updates while actively jamming
  } else { // Not Jamming
    // --- Normal Operation Loop ---
    if (currentTime - lastJammingInputCheckTime > currentInputPollInterval) {
      updateInputs(); // updateInputs handles D-Pad for FIRMWARE_UPDATE_GRID and consumes D-pad flags
      lastJammingInputCheckTime = currentTime;
    }

    if (batteryNeedsUpdate || (currentTime - lastBatteryCheck >= currentBatteryCheckInterval)) {
      getSmoothV();
    }

    // Wi-Fi auto-disable logic
    if (wifiHardwareEnabled && WiFi.status() != WL_CONNECTED) {
        bool inRelevantWifiMenu = (currentMenu == WIFI_SETUP_MENU ||
                                   currentMenu == WIFI_PASSWORD_INPUT ||
                                   currentMenu == WIFI_CONNECTING ||
                                   currentMenu == WIFI_CONNECTION_INFO ||
                                   showWifiDisconnectOverlay);
        
        // MODIFIED: Add condition for OTA_BASIC_ACTIVE when it's in an error state
        // due to no Wi-Fi connection.
        bool inOtaStaMenuWithError = (currentMenu == OTA_BASIC_ACTIVE && otaProgress == -1 && 
                                     (otaStatusMessage.indexOf("Connect to Wi-Fi") != -1 || otaStatusMessage.indexOf("Failed to enable Wi-Fi") != -1) );

        bool inOtaStaMenuReady = (currentMenu == OTA_BASIC_ACTIVE && basicOtaStarted && otaProgress == 0); // Ready and waiting for IDE

        WifiConnectionStatus currentManagerStatus = getCurrentWifiConnectionStatus();
        bool isActivelyTryingToConnect = (wifiIsScanning ||
                                 currentManagerStatus == WIFI_CONNECTING_IN_PROGRESS ||
                                 currentManagerStatus == KIVA_WIFI_SCAN_RUNNING);

        if (!inRelevantWifiMenu && !isActivelyTryingToConnect && !inOtaStaMenuReady && !inOtaStaMenuWithError && !webOtaActive) {
            Serial.println("Loop: Wi-Fi ON, Not Connected, Not in critical Wi-Fi/OTA Menu/State. Disabling Wi-Fi.");
            setWifiHardwareState(false);
        }
    }

    // Wi-Fi Scan & Connection Progress
    if (currentMenu == WIFI_SETUP_MENU && wifiIsScanning && !showWifiDisconnectOverlay) {
      if (currentTime - lastWifiScanCheckTime >= WIFI_SCAN_CHECK_INTERVAL) {
        int result = checkAndRetrieveWifiScanResults();
        if (!wifiIsScanning) {
          initializeCurrentMenu();
          wifiMenuIndex = 0;
          targetWifiListScrollOffset_Y = 0;
          currentWifiListScrollOffset_Y_anim = 0;
          if (maxMenuItems > 0) wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
        }
        lastWifiScanCheckTime = currentTime;
      }
    } else if (currentMenu == WIFI_CONNECTING && !showWifiDisconnectOverlay) {
      WifiConnectionStatus status = checkWifiConnectionProgress();
      if (status != WIFI_CONNECTING_IN_PROGRESS) {
        currentMenu = WIFI_CONNECTION_INFO;
        wifiStatusMessageTimeout = millis() + 3000;
        initializeCurrentMenu();
      }
    } else if (currentMenu == WIFI_CONNECTION_INFO && !showWifiDisconnectOverlay) {
      if (millis() > wifiStatusMessageTimeout) {
        WifiConnectionStatus lastStatus = getCurrentWifiConnectionStatus();
        if (lastStatus == WIFI_CONNECTED_SUCCESS) {
          currentMenu = WIFI_SETUP_MENU;
        } else if (selectedNetworkIsSecure && (lastStatus == WIFI_FAILED_WRONG_PASSWORD || lastStatus == WIFI_FAILED_TIMEOUT)) {
          KnownWifiNetwork* net = findKnownNetwork(currentSsidToConnect);
          if (net && net->failCount >= MAX_WIFI_FAIL_ATTEMPTS) {
            currentMenu = WIFI_PASSWORD_INPUT;
          } else {
            currentMenu = WIFI_SETUP_MENU;
          }
        } else {
          currentMenu = WIFI_SETUP_MENU;
        }
        initializeCurrentMenu();
      }
    }

    // --- Menu Navigation and Selection ---
    bool criticalOtaUpdateInProgress = false;
    if (currentMenu == OTA_WEB_ACTIVE || currentMenu == OTA_SD_STATUS || currentMenu == OTA_BASIC_ACTIVE) {
        if ((otaProgress > 0 && otaProgress < 100) ||
            (otaProgress == 100 && otaStatusMessage.indexOf("Rebooting") != -1)) {
            criticalOtaUpdateInProgress = true;
        }
    }

    bool allowMenuInput = !criticalOtaUpdateInProgress;
    if (showWifiDisconnectOverlay) { // PHASE 2: Will add || showWifiRedirectPromptOverlay
        allowMenuInput = false;
    }

    if (allowMenuInput) {
      bool selectionKeyPressed = false; 
      // encBtnWasForKeyboard is not strictly needed here if updateInputs fully consumes ENC_BTN for keyboard
      // but the refined logic below for selectionKeyPressed handles it implicitly.

      if (btnPress1[NAV_OK]) {
        btnPress1[NAV_OK] = false; 
        selectionKeyPressed = true;
      }
      
      if (btnPress0[ENC_BTN]) {
        if (currentMenu != WIFI_PASSWORD_INPUT) { // Only treat as selection if not password input
          btnPress0[ENC_BTN] = false; 
          selectionKeyPressed = true;
        }
        // If currentMenu IS WIFI_PASSWORD_INPUT, updateInputs() is responsible for handling
        // ENC_BTN for keyboard entry and should consume/clear btnPress0[ENC_BTN] there.
      }

      if (selectionKeyPressed) {
        handleMenuSelection(); // This is the primary action for OK/Select
      }

      if (btnPress1[NAV_BACK]) {
        btnPress1[NAV_BACK] = false; 
        handleMenuBackNavigation();
      }
    }

    // --- General Button Flag Clearing (Safeguards for unconsumed flags) ---
    // PCF0 buttons (btnPress0):
    // ENC_BTN is handled/consumed above if it was a selection or by updateInputs for keyboard.
    // If it's still true here, it was an unhandled press for this cycle.
    // Other PCF0 buttons (BTN_AI, etc.) should be consumed by their specific action handlers.
    // If they are one-shot and not consumed, clearing them here is a fallback.
    if (allowMenuInput && !showWifiDisconnectOverlay /* PHASE 2: && !showWifiRedirectPromptOverlay */) {
        for (int i = 0; i < 8; ++i) {
            if (i == ENC_A || i == ENC_B) continue; // Encoder pins A and B are not one-shot buttons

            if (i == ENC_BTN) {
                // If btnPress0[ENC_BTN] is still true here, it means it wasn't consumed by selection
                // logic above, nor by keyboard input logic in updateInputs().
                // This implies it was an unhandled press.
                if (btnPress0[ENC_BTN]) { // Only clear if still set
                    // Serial.println("Loop: Clearing unhandled ENC_BTN"); // For debugging
                    // btnPress0[ENC_BTN] = false; // Decided against aggressive clear here; rely on specific handlers.
                                                // If it's truly unhandled, it will likely be ignored anyway.
                                                // The selection logic *should* have caught it.
                }
            } else {
                // For other PCF0 buttons like BTN_AI, BTN_RIGHT1, BTN_RIGHT2:
                // If these are intended as one-shot actions and are not consumed elsewhere,
                // clearing them here might be necessary.
                // Example: btnPress0[i] = false;
                // For now, assuming they are handled if used, or their persistence is not problematic.
            }
        }
    }

    // PCF1 D-Pad flags (NAV_UP, NAV_DOWN, NAV_LEFT, NAV_RIGHT):
    // These are cleared in updateInputs() if they cause a scroll action.
    // This is a safeguard if they were pressed but didn't result in scroll (e.g., at menu limits).
    if (!showWifiDisconnectOverlay && currentMenu != WIFI_PASSWORD_INPUT && allowMenuInput /* PHASE 2: && !showWifiRedirectPromptOverlay */) {
      btnPress1[NAV_UP] = false;
      btnPress1[NAV_DOWN] = false;
      btnPress1[NAV_LEFT] = false;
      btnPress1[NAV_RIGHT] = false;
      // NAV_A and NAV_B are typically special function buttons.
      // Their flags should be consumed where their specific action is handled.
      // btnPress1[NAV_A] = false; // Example if needed
      // btnPress1[NAV_B] = false; // Example if needed
    }

    drawUI();
  }

  // Conditional delay AT THE END OF THE LOOP
  if (isJammingOperationActive) {
    delay(JAMMER_CYCLE_DELAY_MS); // e.g., 1ms
  } else if (webOtaActive) {
      // If WebOTA is active, and we are in the upload phase (otaProgress is positive but not 100 or error states)
      // we need to be extremely responsive.
      if (otaProgress > 0 && otaProgress < 100) { // Check if upload/flash is in progress
          delay(0); // Effectively yield(), ensuring handleClient() is called very frequently.
      } else {
          delay(1); // Still very responsive for other WebOTA states (server idle, post-flash)
      }
  } else if (basicOtaStarted) {
    delay(1); // Basic OTA also needs frequent handling
  }
  else {
    delay(16); // Normal loop delay for UI responsiveness
  }
}