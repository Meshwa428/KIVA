#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h> // For standard types like uint8_t, etc.
#include <U8g2lib.h> // For U8G2 types if needed in structs here

// === I2C Multiplexer & Peripherals ===
#define MUX_ADDR 0x70
#define PCF0_ADDR 0x24 // Inputs (Encoder, Buttons) & Outputs (LEDs?)
#define PCF1_ADDR 0x20 // Inputs (Navigation Buttons)

// === Display ===
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE); // Definition will be in KivaMain.ino

// === ADC & Battery ===
#define ADC_PIN 3
#define ADC_REF 3.3f
#define ADC_RES 4095
#define VOLTAGE_RATIO (70.0f / 50.0f)
#define BAT_SAMPLES 20
#define BATTERY_CHECK_INTERVAL 120000UL // ms

// === Buttons (PCF0) ===
#define ENC_BTN 0
#define ENC_A 1
#define ENC_B 2
#define BTN_AI 3       // Example, adjust if used
#define BTN_RIGHT1 4
#define BTN_RIGHT2 5

// === Navigation Buttons (PCF1) ===
#define NAV_OK 0
#define NAV_BACK 1
#define NAV_A 2         // Example, if used for specific actions
#define NAV_B 3         // Example, if used for specific actions
#define NAV_LEFT 4
#define NAV_UP 5
#define NAV_DOWN 6
#define NAV_RIGHT 7

// === Debouncing & Auto-repeat ===
#define DEBOUNCE_DELAY 50UL // ms
#define REPEAT_INIT_DELAY 400UL // ms
#define REPEAT_INT 150UL // ms

// === Menu System ===
enum MenuState {
  MAIN_MENU,
  GAMES_MENU,
  TOOLS_MENU,
  SETTINGS_MENU,
  TOOL_CATEGORY_GRID
  // Add more states for actual game/tool screens later
};

// --- Extern Global Variables (to be defined in KivaMain.ino or a globals.cpp) ---
extern MenuState currentMenu;
extern int menuIndex;
extern int maxMenuItems;
extern int mainMenuSavedIndex;
extern int toolsCategoryIndex;
extern int gridCols;
extern int targetGridScrollOffset_Y;
extern float currentGridScrollOffset_Y_anim;

extern uint8_t pcf0Output; // For controlling outputs on PCF0

// --- UI Layout Constants ---
#define GRID_ITEM_H 18
#define GRID_ITEM_PADDING_Y 4
#define GRID_ITEM_PADDING_X 4
#define STATUS_BAR_H 11

// --- Animation Constants & Structs ---
#define MAX_ANIM_ITEMS 10
#define MAX_GRID_ITEMS 20 
#define GRID_ANIM_STAGGER_DELAY 40.0f
#define GRID_ANIM_SPEED 20.0f

extern float gridItemScale[MAX_GRID_ITEMS];
extern float gridItemTargetScale[MAX_GRID_ITEMS];
extern unsigned long gridItemAnimStartTime[MAX_GRID_ITEMS];
extern bool gridAnimatingIn;

// Forward declare U8G2 type for struct members if they use it directly
// (Not strictly needed here if they only take U8G2& as parameter)
// class U8G2; // Example: U8G2_SH1106_128X64_NONAME_F_HW_I2C;


struct QuadratureEncoder {
  int position;
  int lastState;
  unsigned long lastValidTime;
  int consecutiveValid;
  const unsigned long minInterval = 1;
  const int requiredConsecutive = 2;
  const int cwTable[4] = {1, 3, 0, 2};
  const int ccwTable[4] = {2, 0, 3, 1};

  void init(int iA, int iB);
  bool update(int cA, int cB);
};

struct VerticalListAnimation {
  float itemOffsetY[MAX_ANIM_ITEMS], itemScale[MAX_ANIM_ITEMS];
  float targetOffsetY[MAX_ANIM_ITEMS], targetScale[MAX_ANIM_ITEMS];
  const float animSpd = 12.f, frmTime = 0.016f;
  const float itmSpc = 20.f;

  void init();
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

// --- Shared Buffer for truncateText ---
extern char SBUF[32];


#endif // CONFIG_H