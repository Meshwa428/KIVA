#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include "Config.h" // For the enum definitions

namespace DebugUtils {
    // Converts a MenuType enum to a string for logging
    const char* menuTypeToString(MenuType type);

    // Converts an InputEvent enum to a string for logging
    const char* inputEventToString(InputEvent event);
}

#endif // DEBUG_UTILS_H