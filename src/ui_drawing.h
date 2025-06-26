#ifndef UI_DRAWING_H
#define UI_DRAWING_H

#include "config.h"
#include "firmware_metadata.h" // For FirmwareInfo struct
#include <U8g2lib.h>
#include "icons.h" // <-- Make sure this is included to know IconRenderSize and IconType

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
extern U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2_small;

extern VerticalListAnimation mainMenuAnim;
extern CarouselAnimation subMenuAnim;
extern VerticalListAnimation wifiListAnim;

// Function declarations
void drawUI();
void drawStatusBar();
void drawMainMenu();
void drawCarouselMenu();
void drawToolGridScreen();
void startGridItemAnimation();
void drawPassiveDisplay();

void drawWifiSetupScreen();
void drawWifiSignalStrength(int x, int y, int8_t rssi);

void drawPasswordInputScreen();
void drawKeyboardOnSmallDisplay();
void drawWifiConnectingStatusScreen();
void drawWifiConnectionResultScreen();
void drawPromptOverlay();
void drawJammingActiveScreen();

// --- NEW OTA Drawing Functions ---
void drawOtaStatusScreen(); // Generic screen for Web, SD, Basic progress/status
void drawSDFirmwareListScreen();

void drawRfToolkitOverviewMenu();

void drawRndBox(int x, int y, int w, int h, int r, bool fill);
void drawBatIcon(int x, int y, uint8_t percentage);

// Updated drawCustomIcon declaration
// The 'iconType' is an int here, but it will correspond to the IconType enum values.
// The IconRenderSize enum comes from "icons.h"
void drawCustomIcon(int x_top_left, int y_top_left, int iconType, IconRenderSize size = RENDER_SIZE_LARGE);

void updateMarquee(int cardInnerW, const char* text);
char* truncateText(const char* originalText, int maxWidthPixels, U8G2 &display);

#endif // UI_DRAWING_H