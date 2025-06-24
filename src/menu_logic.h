#ifndef MENU_LOGIC_H
#define MENU_LOGIC_H

#include "config.h" 
#include "firmware_metadata.h" // For FirmwareInfo

// Extern Menu Item Arrays (defined in menu_logic.cpp)
extern const char* mainMenuItems[];
extern const char* rfToolkitMenuItems[];    // <--- NEW
extern const char* wifiSniffingMenuItems[]; // <--- NEW (Example for sub-menu)
extern const char* wifiAttackMenuItems[];   // <--- NEW (Example for sub-menu)
extern const char* gamesMenuItems[];
extern const char* toolsMenuItems[];
extern const char* injectionToolItems[];
extern const char* wifiAttackToolItems[];
extern const char* bleAttackToolItems[];
extern const char* nrfReconToolItems[];
extern const char* jammingToolItems[];
extern const char* settingsMenuItems[];
extern const char* utilitiesMenuItems[];
extern const char* otaMenuItems[]; // <--- NEW
extern FirmwareInfo g_firmwareToInstallFromSd; // Make it accessible to menu_logic.cpp


// --- Getter Functions for Menu Item Counts ---
int getMainMenuItemsCount();
int getRfToolkitMenuItemsCount();    // <--- NEW
int getWifiSniffingMenuItemsCount(); // <--- NEW (Example)
int getWifiAttackMenuItemsCount();   // <--- NEW (Example)
int getGamesMenuItemsCount();
int getToolsMenuItemsCount();
int getSettingsMenuItemsCount();
int getUtilitiesMenuItemsCount();
int getInjectionToolItemsCount();
int getWifiAttackToolItemsCount();
int getBleAttackToolItemsCount();
int getNrfReconToolItemsCount();
int getJammingToolItemsCount();
int getOtaMenuItemsCount(); // <--- NEW

// Function declarations for menu actions
void handleMenuSelection();
void handleMenuBackNavigation();
void initializeCurrentMenu();

void activatePromptOverlay(const char* title, const char* message, 
                           const char* opt0Txt, const char* opt1Txt,
                           MenuState actionMenu = currentMenu, // Default: currentMenu
                           void (*confirmAction)() = nullptr); // Default: nullptr

#endif // MENU_LOGIC_H