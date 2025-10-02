#ifndef SD_CARD_MANAGER_H
#define SD_CARD_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <map>
#include <string>
#include <memory>
#include "ICachedFileReader.h"

namespace SdCardManager {

    // --- NEW: LineReader that uses the abstract reader interface ---
    class LineReader {
    public:
        LineReader();
        ~LineReader();
        LineReader(LineReader&& other) noexcept;
        LineReader& operator=(LineReader&& other) noexcept;

        String readLine();
        bool isOpen() const;
        void close();

        LineReader(const LineReader&) = delete;
        LineReader& operator=(const LineReader&) = delete;

    private:
        friend class SdCardManagerAPI; // Allow factory to create this
        LineReader(std::unique_ptr<ICachedFileReader> reader);
        std::unique_ptr<ICachedFileReader> reader_;
    };
    
    // --- NEW: A class to hold all the public API and internal cache state ---
    class SdCardManagerAPI {
    public:
        SdCardManagerAPI();

        bool setup();
        bool isAvailable() const;
        bool exists(const char* path);
        bool createDir(const char* path);
        String readFile(const char* path); // Now uses cache
        bool writeFile(const char* path, const char* message);
        bool deleteFile(const char *path);
        bool renameFile(const char* pathFrom, const char* pathTo);
        void ensureStandardDirs();
        
        // --- REVISED: These now return the abstract interface ---
        std::unique_ptr<ICachedFileReader> open(const char* path);
        LineReader openLineReader(const char* path);

        // --- UNCACHED: For binary streaming where caching is wasteful (OTA) ---
        File openFileUncached(const char* path, const char* mode = FILE_READ);
    
    private:
        // --- CACHE IMPLEMENTATION ---
        struct CachedFile {
            std::shared_ptr<char> data; // Use shared_ptr for automatic memory management
            size_t size;
            unsigned long lastAccess;

            // Custom deleter for ps_malloc'd memory
            struct PsramDeleter {
                void operator()(char* p) const {
                    if (p) free(p);
                }
            };

            CachedFile(char* d, size_t s) 
                : data(d, PsramDeleter()), size(s), lastAccess(millis()) {}
        };
        
        std::map<std::string, CachedFile> cache_;
        size_t currentCacheSize_ = 0;

        void cacheFile(const std::string& path);
        void evictLRU();
        void touch(const std::string& path);

        bool sdCardInitialized_ = false;

        // --- CACHE CONFIGURATION ---
        static constexpr size_t MAX_CACHEABLE_FILE_SIZE = 6 * 1024 * 1024; // 6 MB
        static constexpr size_t MAX_TOTAL_CACHE_SIZE = 7 * 1024 * 1024;    // 7 MB
    };

    // --- Provide a single global instance of the API ---
    SdCardManagerAPI& getInstance();

} // namespace SdCardManager

#endif // SD_CARD_MANAGER_H