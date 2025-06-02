#include <Wire.h>
#include "config.h"
#include "keyboard_layout.h" // Include for KeyboardLayer enum and layouts
#include "pcf_utils.h"
#include "input_handling.h"
#include "battery_monitor.h"
#include "ui_drawing.h"
#include "menu_logic.h"
#include "wifi_manager.h"
#include "sd_card_manager.h"

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


// === Small Display Boot Log Globals === (no change)
#define MAX_LOG_LINES_SMALL_DISPLAY 4
#define MAX_LOG_LINE_LENGTH_SMALL_DISPLAY 32
char smallDisplayLogBuffer[MAX_LOG_LINES_SMALL_DISPLAY][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY];

// === Boot Progress Globals (MODIFIED) ===
int totalBootSteps = 0;
int currentBootStep = 0;
float currentProgressBarFillPx_anim = 0.0f; // Current animated fill width in pixels

// Variables for per-segment animation
unsigned long segmentAnimStartTime = 0;     // When the current segment animation began
float segmentAnimDurationMs_float = 300.0f; // Duration of one segment's animation (matches stepDisplayDurationMs)
float segmentAnimStartPx = 0.0f;            // Fill Px at the start of current segment animation
float segmentAnimTargetPx = 0.0f;           // Target Fill Px for the current segment

const int PROGRESS_ANIM_UPDATE_INTERVAL_MS = 16; // Approx 60 FPS updates for animation


void updateMainDisplayBootProgress() {
    selectMux(MUX_CHANNEL_MAIN_DISPLAY);

    // --- Smooth Easing Animation Logic for Progress Bar ---
    unsigned long currentTime = millis();
    float elapsedSegmentTime = 0;

    if (segmentAnimStartTime > 0 && segmentAnimDurationMs_float > 0) {
        elapsedSegmentTime = (float)(currentTime - segmentAnimStartTime);
    }

    float progress_t = 0.0f; // Time progress for current segment (0.0 to 1.0)
    if (segmentAnimDurationMs_float > 0) {
        progress_t = elapsedSegmentTime / segmentAnimDurationMs_float;
    }
    
    if (progress_t < 0.0f) progress_t = 0.0f;
    if (progress_t > 1.0f) progress_t = 1.0f;

    // Ease-Out Cubic function: f(t) = 1 - (1-t)^3
    // Gives a nice effect of starting faster and slowing down.
    float eased_t = 1.0f - pow(1.0f - progress_t, 3);

    currentProgressBarFillPx_anim = segmentAnimStartPx + (segmentAnimTargetPx - segmentAnimStartPx) * eased_t;
    // --- End Smooth Easing Animation Logic ---

    u8g2.firstPage();
    do {
        if (sizeof(im_bits) > 1) {
            u8g2.drawXBM(0, 0, im_width, im_height, im_bits);
        } else {
            u8g2.setFont(u8g2_font_ncenB10_tr);
            u8g2.drawStr((128 - u8g2.getStrWidth("KIVA")) / 2, 35, "KIVA");
        }

        int progressBarY = im_height - 12 + 2;
        int progressBarHeight = 7;
        int progressBarWidth = im_width - 40;
        int progressBarX = (im_width - progressBarWidth) / 2;
        int progressBarDrawableWidth = progressBarWidth - 2;

        u8g2.drawRFrame(progressBarX, progressBarY, progressBarWidth, progressBarHeight, 1);

        int fillWidthToDraw = (int)round(currentProgressBarFillPx_anim); // Round for pixel drawing
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

  selectMux(MUX_CHANNEL_SECOND_DISPLAY);
  u8g2_small.begin();
  u8g2_small.enableUTF8Print();
  u8g2_small.clearBuffer();
  u8g2_small.sendBuffer();

  selectMux(MUX_CHANNEL_MAIN_DISPLAY);
  u8g2.begin();
  u8g2.enableUTF8Print();

  totalBootSteps = 9;
  currentBootStep = 0;
  currentProgressBarFillPx_anim = 0.0f; // Start animation from 0
  segmentAnimStartPx = 0.0f;
  segmentAnimTargetPx = 0.0f;
  segmentAnimStartTime = 0; // Will be set before first animation

  // This duration will be used for each segment's animation.
  const int stepDisplayDurationMs = 350; // How long each *logical* step message is shown AND animated for
  segmentAnimDurationMs_float = (float)stepDisplayDurationMs;


  updateMainDisplayBootProgress(); // Initial draw (empty bar)
  delay(100);

  // --- Helper lambda to manage a single boot step's animation ---
  auto runBootStepAnimation = [&](const char* logMsg, const char* logStatus) {
    logToSmallDisplay(logMsg, logStatus);
    currentBootStep++;
    
    segmentAnimStartPx = currentProgressBarFillPx_anim; // Start animation from where the bar currently is
    int progressBarTotalDrawableWidth = (im_width - 40 - 2);
    segmentAnimTargetPx = (totalBootSteps > 0) ? (float)(progressBarTotalDrawableWidth * currentBootStep) / totalBootSteps : 0.0f;
    
    segmentAnimStartTime = millis();
    unsigned long currentSegmentLoopStartTime = millis();

    while (millis() - currentSegmentLoopStartTime < stepDisplayDurationMs) {
        updateMainDisplayBootProgress();
        delay(PROGRESS_ANIM_UPDATE_INTERVAL_MS);
    }
    // Ensure the animation finishes at the exact target for this step
    currentProgressBarFillPx_anim = segmentAnimTargetPx; 
    updateMainDisplayBootProgress(); // Final draw for this step
  };

  // --- Step-by-Step Initialization using the helper ---
  runBootStepAnimation("Kiva Boot Agent v1.0", NULL);
  runBootStepAnimation("I2C Bus Initialized", "OK");
  
  selectMux(0);
  writePCF(PCF0_ADDR, pcf0Output);
  runBootStepAnimation("PCF0 & MUX Setup", "OK");

  bool sdInitializedResult = setupSdCard();
  runBootStepAnimation("SD Card Mounted", sdInitializedResult ? "PASS" : "FAIL");

  analogReadResolution(12);
  #if defined(ESP32) || defined(ESP_PLATFORM)
  // analogSetPinAttenuation((uint8_t)ADC_PIN, ADC_11DB);
  #endif
  setupBatteryMonitor();
  runBootStepAnimation("Battery Service", "OK");

  setupInputs();
  runBootStepAnimation("Input Handler", "OK");
  
  setupWifi();
  runBootStepAnimation("WiFi Subsystem", "INIT");

  runBootStepAnimation("Main Display OK", "READY");

  // For the last step, "KivaOS Loading...", give it a bit more visual time
  logToSmallDisplay("KivaOS Loading...", NULL);
  currentBootStep++;
  segmentAnimStartPx = currentProgressBarFillPx_anim;
  int progressBarTotalDrawableWidth = (im_width - 40 - 2);
  segmentAnimTargetPx = (totalBootSteps > 0) ? (float)(progressBarTotalDrawableWidth * currentBootStep) / totalBootSteps : 0.0f;
  segmentAnimStartTime = millis();
  unsigned long finalStepLoopStartTime = millis();
  while (millis() - finalStepLoopStartTime < (stepDisplayDurationMs + 200)) { // Slightly longer
      updateMainDisplayBootProgress();
      delay(PROGRESS_ANIM_UPDATE_INTERVAL_MS);
  }
  currentProgressBarFillPx_anim = segmentAnimTargetPx;
  updateMainDisplayBootProgress();
  delay(300); // Pause to show the full bar

  initializeCurrentMenu();
  wifiPasswordInput[0] = '\0';
}


void loop() {
  updateInputs();

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
        setWifiHardwareState(false);
    }
  }


  if (currentMenu == WIFI_SETUP_MENU && wifiIsScanning && !showWifiDisconnectOverlay) { 
    if (millis() - lastWifiScanCheckTime > WIFI_SCAN_CHECK_INTERVAL) {
      int scanCompleteResult = WiFi.scanComplete();

      if (scanCompleteResult >= 0) {
        wifiIsScanning = false;
        checkAndRetrieveWifiScanResults();
        initializeCurrentMenu();
        wifiMenuIndex = 0;
        targetWifiListScrollOffset_Y = 0;
        currentWifiListScrollOffset_Y_anim = 0;
        if (currentMenu == WIFI_SETUP_MENU) {
             wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
        }
      } else if (scanCompleteResult == WIFI_SCAN_FAILED) {
        Serial.println("Loop: Scan failed as per WiFi.scanComplete()");
        wifiIsScanning = false;
        foundWifiNetworksCount = 0;
        checkAndRetrieveWifiScanResults();
        initializeCurrentMenu();
        wifiMenuIndex = 0; targetWifiListScrollOffset_Y = 0; currentWifiListScrollOffset_Y_anim = 0;
        if(currentMenu == WIFI_SETUP_MENU) wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
      }
      lastWifiScanCheckTime = millis();
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


  if (!showWifiDisconnectOverlay) { 
      if (btnPress1[NAV_OK] || btnPress0[ENC_BTN]) {
        if (btnPress1[NAV_OK]) btnPress1[NAV_OK] = false;
        if (btnPress0[ENC_BTN]) btnPress0[ENC_BTN] = false;
        if (currentMenu != WIFI_PASSWORD_INPUT) {
            handleMenuSelection();
        }
      }

      if (btnPress1[NAV_BACK]) {
        btnPress1[NAV_BACK] = false;
        handleMenuBackNavigation();
      }
  }


  for (int i = 0; i < 8; ++i) {
      if ((currentMenu == WIFI_PASSWORD_INPUT || showWifiDisconnectOverlay) && i == ENC_BTN) {
          // Don't clear ENC_BTN
      } else {
          btnPress0[i] = false;
      }
      if (!showWifiDisconnectOverlay || (i != NAV_OK && i != NAV_BACK && i != NAV_LEFT && i != NAV_RIGHT)) {
        // This logic for btnPress1 seems a bit off, as it's not indexed by i directly for NAV buttons.
        // The below specific clear for NAV_UP/DOWN etc. is more targeted.
      }
  }
  
  if (!showWifiDisconnectOverlay && currentMenu != WIFI_PASSWORD_INPUT) {
    // These are cleared if their one-shot press isn't consumed by keyboard or scrollAct for non-repeated scroll
    btnPress1[NAV_UP] = false;
    btnPress1[NAV_DOWN] = false;
    btnPress1[NAV_LEFT] = false; 
    btnPress1[NAV_RIGHT] = false; 
  }
  // NAV_OK and NAV_BACK flags are cleared where they are handled (menu selection, overlay, keyboard)


  drawUI();
  delay(16);
}