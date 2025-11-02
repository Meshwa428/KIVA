#ifndef RESOURCE_H
#define RESOURCE_H

#include <cstdint>

// Strongly-typed enum for resource requirement bitmasks
enum class ResourceRequirement : uint32_t {
    NONE            = 0,
    WIFI            = 1 << 0,
    BLE             = 1 << 1,
    HOST_PERIPHERAL = 1 << 2, // Covers USB or BLE HID
    NRF24           = 1 << 3,
    SD_CARD         = 1 << 4, // For future use
    AUDIO           = 1 << 5, // For future use
};

#endif // RESOURCE_H
