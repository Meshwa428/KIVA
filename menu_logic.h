#ifndef MENU_LOGIC_H
#define MENU_LOGIC_H

#include "config.h" 
#include "ui_drawing.h" 

// Extern Menu Item Arrays (defined in menu_logic.cpp)
extern const char* mainMenuItems[];
extern const char* gamesMenuItems[];
extern const char* toolsMenuItems[];
extern const char* injectionToolItems[];
extern const char* wifiAttackToolItems[];
extern const char* bleAttackToolItems[];
extern const char* nrfReconToolItems[];
extern const char* jammingToolItems[];
extern const char* settingsMenuItems[];

// --- Corresponding Sizes for Menu Item Arrays --- <--- ADD THIS SECTION
// These must be updated MANUALLY if you change the number of items in the arrays in menu_logic.cpp
constexpr int MAIN_MENU_ITEMS_COUNT = 4;
constexpr int GAMES_MENU_ITEMS_COUNT = 5;
constexpr int TOOLS_MENU_ITEMS_COUNT = 6;
constexpr int INJECTION_TOOL_ITEMS_COUNT = 6;
constexpr int WIFI_ATTACK_TOOL_ITEMS_COUNT = 9;
constexpr int BLE_ATTACK_TOOL_ITEMS_COUNT = 6;
constexpr int NRF_RECON_TOOL_ITEMS_COUNT = 9;
constexpr int JAMMING_TOOL_ITEMS_COUNT = 6;
constexpr int SETTINGS_MENU_ITEMS_COUNT = 5;


// Function declarations for menu actions
void handleMenuSelection();
void handleMenuBackNavigation();
void initializeCurrentMenu(); 


#endif // MENU_LOGIC_H