#ifndef GENERIC_DATE_TIME_H
#define GENERIC_DATE_TIME_H

#include <cstdint>

// A simple, general-purpose struct to hold date and time components.
struct GenericDateTime {
    uint16_t y;
    uint8_t m, d, h, min, s;
};

#endif // GENERIC_DATE_TIME_H
