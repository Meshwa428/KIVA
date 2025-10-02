#ifndef I_CACHED_FILE_READER_H
#define I_CACHED_FILE_READER_H

#include <cstddef>
#include <Arduino.h> // For String

// An abstract interface for reading file-like data,
// which could be from an SD card or a PSRAM buffer.
class ICachedFileReader {
public:
    virtual ~ICachedFileReader() = default;

    virtual size_t read(uint8_t* buf, size_t size) = 0;
    virtual bool seek(uint32_t pos) = 0;
    virtual size_t available() = 0;
    virtual size_t size() const = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual String readStringUntil(char terminator) = 0;
};

#endif // I_CACHED_FILE_READER_H