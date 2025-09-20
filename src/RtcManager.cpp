#include "RtcManager.h"
#include "App.h"
#include "Logger.h"
#include "Config.h"
#include "time.h"
#include <Wire.h>

#define DS3231_ADDR 0x68

RtcManager::RtcManager() : app_(nullptr), rtcFound_(false), lastNtpSyncTime_(0) {}

// --- Helper Functions ---

uint8_t RtcManager::bcdToDec(uint8_t val) {
    return ((val / 16 * 10) + (val % 16));
}

uint8_t RtcManager::decToBcd(uint8_t val) {
    return ((val / 10 * 16) + (val % 10));
}

// --- Private Methods ---

void RtcManager::selectRtcMux() {
    app_->getHardwareManager().selectMux(Pins::MUX_CHANNEL_RTC);
}

void RtcManager::adjust(const GenericDateTime& dt) {
    if (!rtcFound_) return;
    selectRtcMux();
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00); // Start at register 0
    Wire.write(decToBcd(dt.s));
    Wire.write(decToBcd(dt.min));
    Wire.write(decToBcd(dt.h));
    Wire.write(decToBcd(0)); // Day of week (not used)
    Wire.write(decToBcd(dt.d));
    Wire.write(decToBcd(dt.m));
    Wire.write(decToBcd(dt.y - 2000));
    Wire.endTransmission();
}

// --- Public Methods ---

void RtcManager::setup(App* app) {
    app_ = app;
    selectRtcMux();
    Wire.beginTransmission(DS3231_ADDR);
    if (Wire.endTransmission() == 0) {
        rtcFound_ = true;
        LOG(LogLevel::INFO, "RTC", "DS3231 RTC found.");

        // Check Oscillator Stop Flag (OSF) to see if power was lost
        selectRtcMux();
        Wire.beginTransmission(DS3231_ADDR);
        Wire.write(0x0F); // Status register
        Wire.endTransmission();
        Wire.requestFrom(DS3231_ADDR, 1);
        uint8_t status = Wire.read();

        if (status & 0x80) {
            LOG(LogLevel::WARN, "RTC", "RTC lost power (OSF set). Setting time to firmware compile time.");
            // Use compile time to set RTC
            GenericDateTime compiled_dt;
            const char* comp_date = __DATE__;
            const char* comp_time = __TIME__;
            char month_str[4];
            sscanf(comp_date, "%s %hhu %hu", month_str, &compiled_dt.d, &compiled_dt.y);
            sscanf(comp_time, "%hhu:%hhu:%hhu", &compiled_dt.h, &compiled_dt.min, &compiled_dt.s);
            const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
            for(int i=0; i<12; ++i) {
                if(strcmp(month_str, months[i]) == 0) {
                    compiled_dt.m = i + 1;
                    break;
                }
            }
            adjust(compiled_dt);

            // Clear the OSF flag
            selectRtcMux();
            Wire.beginTransmission(DS3231_ADDR);
            Wire.write(0x0F);
            Wire.write(status & ~0x80);
            Wire.endTransmission();
        }

    } else {
        rtcFound_ = false;
        LOG(LogLevel::ERROR, "RTC", "DS3231 RTC not found!");
    }
}

void RtcManager::update() {}

bool RtcManager::isRtcFound() const {
    return rtcFound_;
}

GenericDateTime RtcManager::now() {
    GenericDateTime dt = {0,0,0,0,0,0};
    if (!rtcFound_) return dt;

    selectRtcMux();
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDR, 7);
    dt.s = bcdToDec(Wire.read() & 0x7F);
    dt.min = bcdToDec(Wire.read());
    dt.h = bcdToDec(Wire.read() & 0x3F);
    Wire.read(); // Skip day of week
    dt.d = bcdToDec(Wire.read());
    dt.m = bcdToDec(Wire.read() & 0x1F);
    dt.y = bcdToDec(Wire.read()) + 2000;

    return dt;
}

std::string RtcManager::getFormattedTime() {
    if (!rtcFound_) return "--:--";
    GenericDateTime dt = now();
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", dt.h, dt.min);
    return std::string(buf);
}

std::string RtcManager::getFormattedDate() {
    if (!rtcFound_) return "----/--/--";
    GenericDateTime dt = now();
    char buf[11];
    snprintf(buf, sizeof(buf), "%04d/%02d/%02d", dt.y, dt.m, dt.d);
    return std::string(buf);
}

void RtcManager::onNtpSync() {
    if (!rtcFound_ || app_->getWifiManager().getState() != WifiState::CONNECTED) return;
    if (millis() - lastNtpSyncTime_ < 600000) return; // 10 minutes

    LOG(LogLevel::INFO, "RTC", "Attempting NTP time synchronization...");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10000)) { // 10 second timeout
        GenericDateTime ntp_dt = {
            (uint16_t)(timeinfo.tm_year + 1900),
            (uint8_t)(timeinfo.tm_mon + 1),
            (uint8_t)timeinfo.tm_mday,
            (uint8_t)timeinfo.tm_hour,
            (uint8_t)timeinfo.tm_min,
            (uint8_t)timeinfo.tm_sec
        };
        adjust(ntp_dt);
        LOG(LogLevel::INFO, "RTC", "NTP sync successful. RTC updated.");
        lastNtpSyncTime_ = millis();
    } else {
        LOG(LogLevel::ERROR, "RTC", "NTP sync failed.");
    }
}