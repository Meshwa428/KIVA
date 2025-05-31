#ifndef MENU_LOGIC_H
#define MENU_LOGIC_H

#include "config.h" 

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
extern const char* utilitiesMenuItems[]; // <--- NEW

// --- Getter Functions for Menu Item Counts ---
int getMainMenuItemsCount();
int getGamesMenuItemsCount();
int getToolsMenuItemsCount();
int getSettingsMenuItemsCount();
int getUtilitiesMenuItemsCount(); // <--- NEW
int getInjectionToolItemsCount();
int getWifiAttackToolItemsCount();
int getBleAttackToolItemsCount();
int getNrfReconToolItemsCount();
int getJammingToolItemsCount();
// Add more getters if other arrays need their size externally

// Function declarations for menu actions
void handleMenuSelection();
void handleMenuBackNavigation();
void initializeCurrentMenu();

#endif // MENU_LOGIC_H