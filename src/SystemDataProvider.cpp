#include "SystemDataProvider.h"
#include "App.h"
#include "SdCardManager.h"
#include <esp_system.h>
#include <esp_psram.h>
#include <Arduino.h> // For temperatureRead()

SystemDataProvider::SystemDataProvider() :
    app_(nullptr),
    lastUpdateTime_(0),
    cpuFrequency_(0),
    temperature_(0.0f)
{
    ramUsage_ = {0, 0, 0};
    psramUsage_ = {0, 0, 0};
    sdCardUsage_ = {0, 0, 0};
}

void SystemDataProvider::setup(App* app) {
    app_ = app;
    updateData(); // Initial data fetch
}

void SystemDataProvider::update() {
    if (millis() - lastUpdateTime_ > 1000) { // Update every second
        updateData();
    }
}

void SystemDataProvider::updateData() {
    // RAM
    ramUsage_.total = ESP.getHeapSize();
    ramUsage_.used = ramUsage_.total - ESP.getFreeHeap();
    ramUsage_.percentage = (ramUsage_.total > 0) ? (uint8_t)((ramUsage_.used * 100) / ramUsage_.total) : 0;

    // PSRAM
    if (psramFound()) {
        psramUsage_.total = ESP.getPsramSize();
        psramUsage_.used = psramUsage_.total - ESP.getFreePsram();
        psramUsage_.percentage = (psramUsage_.total > 0) ? (uint8_t)((psramUsage_.used * 100) / psramUsage_.total) : 0;
    } else {
        psramUsage_ = {0, 0, 0};
    }

    // SD Card
    if (SdCardManager::isAvailable()) {
        sdCardUsage_.total = SD.cardSize();
        sdCardUsage_.used = SD.usedBytes();
        sdCardUsage_.percentage = (sdCardUsage_.total > 0) ? (uint8_t)((sdCardUsage_.used * 100) / sdCardUsage_.total) : 0;
    } else {
        sdCardUsage_ = {0, 0, 0};
    }

    // CPU
    cpuFrequency_ = getCpuFrequencyMhz();

    // Temperature
    temperature_ = temperatureRead();

    lastUpdateTime_ = millis();
}

const MemoryUsage& SystemDataProvider::getRamUsage() const { return ramUsage_; }
const MemoryUsage& SystemDataProvider::getPsramUsage() const { return psramUsage_; }
const MemoryUsage& SystemDataProvider::getSdCardUsage() const { return sdCardUsage_; }
uint32_t SystemDataProvider::getCpuFrequency() const { return cpuFrequency_; }
float SystemDataProvider::getTemperature() const { return temperature_; }

std::string SystemDataProvider::formatBytes(uint64_t bytes) {
    char buffer[20];
    const uint64_t KILO = 1024;
    const uint64_t MEGA = KILO * 1024;
    const uint64_t GIGA = MEGA * 1024;

    if (bytes >= GIGA) {
        snprintf(buffer, sizeof(buffer), "%.2fG", (double)bytes / GIGA);
    } else if (bytes >= MEGA) {
        snprintf(buffer, sizeof(buffer), "%.1fM", (double)bytes / MEGA);
    } else {
        snprintf(buffer, sizeof(buffer), "%lluK", bytes / KILO);
    }
    return std::string(buffer);
}