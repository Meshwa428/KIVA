#include "SdCardManager.h"
#include "Config.h"
#include "Logger.h"
#include <vector>
#include <algorithm>

namespace SdCardManager {

    // --- PsramFileReader Implementation (reads from a buffer in PSRAM) ---
    class PsramFileReader : public ICachedFileReader {
    public:
        PsramFileReader(std::shared_ptr<char> data, size_t size)
            : data_(data), size_(size), position_(0) {}

        size_t read(uint8_t* buf, size_t size) override {
            if (!data_ || position_ >= size_) return 0;
            size_t bytes_to_read = std::min(size, size_ - position_);
            memcpy(buf, data_.get() + position_, bytes_to_read);
            position_ += bytes_to_read;
            return bytes_to_read;
        }
        bool seek(uint32_t pos) override {
            if (!data_ || pos >= size_) return false;
            position_ = pos;
            return true;
        }
        size_t available() override {
            if (!data_) return 0;
            return size_ - position_;
        }
        size_t size() const override { return size_; }
        void close() override { data_.reset(); }
        bool isOpen() const override { return (bool)data_; }

        String readStringUntil(char terminator) override {
            if (!available()) return "";
            const char* start = data_.get() + position_;
            const char* end = (const char*)memchr(start, terminator, available());
            size_t len = (end == nullptr) ? available() : (end - start);
            String result(start, len);
            position_ += len;
            if (end != nullptr) {
                position_++; // Skip terminator
            }
            return result;
        }

    private:
        std::shared_ptr<char> data_;
        size_t size_;
        size_t position_;
    };

    // --- SdFileReader Implementation (wraps a real SD File) ---
    class SdFileReader : public ICachedFileReader {
    public:
        SdFileReader(File file) : file_(file) {}
        ~SdFileReader() override { close(); }

        size_t read(uint8_t* buf, size_t size) override { return file_ ? file_.read(buf, size) : 0; }
        bool seek(uint32_t pos) override { return file_ ? file_.seek(pos) : false; }
        size_t available() override { return file_ ? file_.available() : 0; }
        size_t size() const override { return file_ ? file_.size() : 0; }
        void close() override { if (file_) file_.close(); }
        bool isOpen() const override { return (bool)file_; }
        String readStringUntil(char terminator) override { return file_ ? file_.readStringUntil(terminator) : ""; }

    private:
        File file_;
    };

    // --- LineReader Implementation ---
    LineReader::LineReader() : reader_(nullptr) {}
    LineReader::LineReader(std::unique_ptr<ICachedFileReader> reader) : reader_(std::move(reader)) {}
    LineReader::~LineReader() { close(); }
    LineReader::LineReader(LineReader&& other) noexcept : reader_(std::move(other.reader_)) {}
    LineReader& LineReader::operator=(LineReader&& other) noexcept {
        if (this != &other) {
            close();
            reader_ = std::move(other.reader_);
        }
        return *this;
    }
    bool LineReader::isOpen() const { return reader_ && reader_->isOpen(); }
    void LineReader::close() { if (reader_) reader_->close(); }

    String LineReader::readLine() {
        if (!isOpen()) return "";
        while (isOpen() && reader_->available()) {
            String line = reader_->readStringUntil('\n');
            line.trim();
            if (!line.isEmpty()) {
                return line;
            }
        }
        return "";
    }
    
    // --- SdCardManagerAPI Implementation ---
    
    SdCardManagerAPI& getInstance() {
        static SdCardManagerAPI instance;
        return instance;
    }

    SdCardManagerAPI::SdCardManagerAPI() {}
    
    bool SdCardManagerAPI::setup() {
        // MODIFICATION: Use the high-speed begin() overload.
        if (!SD.begin(Pins::SD_CS_PIN, SPI, 40000000)) {
            LOG(LogLevel::ERROR, "SD_MGR", "SD Card Mount Failed.");
            sdCardInitialized_ = false;
            return false;
        }
        sdCardInitialized_ = true;
        LOG(LogLevel::INFO, "SD_MGR", "SD Card Mounted at 40MHz.");
        ensureStandardDirs();
        return true;
    }

    bool SdCardManagerAPI::isAvailable() const { return sdCardInitialized_; }
    bool SdCardManagerAPI::exists(const char* path) { return sdCardInitialized_ && SD.exists(path); }

    File SdCardManagerAPI::openFileUncached(const char* path, const char* mode) {
        if (!sdCardInitialized_) return File();
        File f = SD.open(path, mode);
        // MODIFICATION: Set the larger buffer size on the newly opened file.
        if (f) {
            f.setBufferSize(8192);
        }
        return f;
    }


    std::unique_ptr<ICachedFileReader> SdCardManagerAPI::open(const char* path) {
        if (!sdCardInitialized_) return nullptr;

        std::string path_str(path);
        auto it = cache_.find(path_str);

        // --- CACHE HIT ---
        if (it != cache_.end()) {
            LOG(LogLevel::DEBUG, "SD_CACHE", false, "CACHE HIT for: %s", path);
            touch(path_str);
            return std::make_unique<PsramFileReader>(it->second.data, it->second.size);
        }

        // --- CACHE MISS ---
        File f = SD.open(path, FILE_READ);
        if (!f || f.isDirectory()) {
            if (f) f.close();
            return nullptr;
        }

        size_t fileSize = f.size();
        if (fileSize > 0 && fileSize < MAX_CACHEABLE_FILE_SIZE) {
            f.close(); // Close before caching
            LOG(LogLevel::DEBUG, "SD_CACHE", false, "CACHE MISS for: %s. Caching it.", path);
            cacheFile(path_str);
            // Try to read from cache again
            it = cache_.find(path_str);
            if (it != cache_.end()) {
                touch(path_str);
                return std::make_unique<PsramFileReader>(it->second.data, it->second.size);
            }
        }
        
        // File is too large or caching failed, return direct SD reader with large buffer
        LOG(LogLevel::DEBUG, "SD_CACHE", false, "CACHE SKIP (too large) for: %s", path);
        f.setBufferSize(8192); // Also set buffer for direct reads
        return std::make_unique<SdFileReader>(f);
    }

    LineReader SdCardManagerAPI::openLineReader(const char* path) {
        return LineReader(open(path));
    }

    String SdCardManagerAPI::readFile(const char* path) {
        auto reader = open(path);
        if (!reader || !reader->isOpen()) return "";

        size_t size = reader->size();
        String content;
        content.reserve(size);
        char buffer[256];
        size_t bytesRead;
        while ((bytesRead = reader->read((uint8_t*)buffer, sizeof(buffer))) > 0) {
            content.concat(buffer, bytesRead);
        }
        return content;
    }

    void SdCardManagerAPI::cacheFile(const std::string& path) {
        File f = SD.open(path.c_str(), FILE_READ);
        if (!f || f.isDirectory()) {
            if (f) f.close();
            return;
        }

        size_t fileSize = f.size();
        if (fileSize == 0 || fileSize >= MAX_CACHEABLE_FILE_SIZE) {
            f.close();
            return;
        }

        while (currentCacheSize_ + fileSize > MAX_TOTAL_CACHE_SIZE) {
            evictLRU();
        }

        char* buffer = (char*)ps_malloc(fileSize);
        if (!buffer) {
            LOG(LogLevel::ERROR, "SD_CACHE", "ps_malloc failed for %d bytes!", fileSize);
            f.close();
            return;
        }

        f.read((uint8_t*)buffer, fileSize);
        f.close();

        cache_.emplace(path, CachedFile(buffer, fileSize));
        currentCacheSize_ += fileSize;
        LOG(LogLevel::INFO, "SD_CACHE", "Cached %s (%d bytes). Total cache size: %d / %d bytes.", path.c_str(), fileSize, currentCacheSize_, MAX_TOTAL_CACHE_SIZE);
    }
    
    void SdCardManagerAPI::evictLRU() {
        if (cache_.empty()) return;

        std::string lru_path;
        unsigned long oldest_time = -1;

        for (const auto& pair : cache_) {
            if (pair.second.lastAccess < oldest_time) {
                oldest_time = pair.second.lastAccess;
                lru_path = pair.first;
            }
        }
        
        if (!lru_path.empty()) {
            auto it = cache_.find(lru_path);
            if (it != cache_.end()) {
                size_t fileSize = it->second.size;
                LOG(LogLevel::INFO, "SD_CACHE", "Evicting %s to free %d bytes.", lru_path.c_str(), fileSize);
                currentCacheSize_ -= fileSize;
                cache_.erase(it);
            }
        }
    }

    void SdCardManagerAPI::touch(const std::string& path) {
        auto it = cache_.find(path);
        if (it != cache_.end()) {
            it->second.lastAccess = millis();
        }
    }

    // --- Passthrough functions that don't need caching logic ---
    bool SdCardManagerAPI::writeFile(const char *path, const char *message) { if (!sdCardInitialized_) return false; File f = SD.open(path, FILE_WRITE); if (!f) return false; bool success = f.print(message); f.close(); return success; }
    bool SdCardManagerAPI::deleteFile(const char* path) { if (!sdCardInitialized_) return false; return SD.remove(path); }
    bool SdCardManagerAPI::renameFile(const char* pathFrom, const char* pathTo) { if (!sdCardInitialized_) return false; return SD.rename(pathFrom, pathTo); }
    bool SdCardManagerAPI::createDir(const char *path) { if (!sdCardInitialized_) return false; return SD.mkdir(path); }
    void SdCardManagerAPI::ensureStandardDirs() {
        if (!sdCardInitialized_) return;
        const char* dirs[] = {
            SD_ROOT::CONFIG,
            SD_ROOT::DATA,
            SD_ROOT::USER,
            SD_ROOT::FIRMWARE,
            SD_ROOT::WEB,
            SD_ROOT::DATA_LOGS, 
            SD_ROOT::DATA_CAPTURES,
            SD_ROOT::DATA_PROBES,
            SD_ROOT::USER_BEACON_LISTS,
            SD_ROOT::USER_PORTALS,
            SD_ROOT::USER_DUCKY,
            SD_ROOT::USER_MUSIC,
            SD_ROOT::DATA_GAMES
        };
        for (const char* dir : dirs) { if (!exists(dir)) createDir(dir); }
    }

} // namespace SdCardManager