#include "RtcManager.h"
#include "App.h"
#include "Logger.h"
#include "Config.h"
#include <Wire.h>
#include <sys/time.h> // Required for settimeofday()

#define DS3231_ADDR 0x68

RtcManager::RtcManager() : app_(nullptr), rtcFound_(false), lastNtpSyncTime_(0), lastAttemptTime_(0) {}

uint8_t RtcManager::bcdToDec(uint8_t val) {
    return ((val / 16 * 10) + (val % 16));
}

uint8_t RtcManager::decToBcd(uint8_t val) {
    return ((val / 10 * 16) + (val % 10));
}

void RtcManager::selectRtcMux() {
    app_->getHardwareManager().selectMux(Pins::MUX_CHANNEL_RTC);
}

void RtcManager::adjust(const GenericDateTime& dt) {
    if (!rtcFound_) return;
    selectRtcMux();
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);
    Wire.write(decToBcd(dt.s));
    Wire.write(decToBcd(dt.min));
    Wire.write(decToBcd(dt.h));
    Wire.write(decToBcd(0));
    Wire.write(decToBcd(dt.d));
    Wire.write(decToBcd(dt.m));
    Wire.write(decToBcd(dt.y - 2000));
    Wire.endTransmission();
}

void RtcManager::checkForRtc() {
    selectRtcMux();
    Wire.beginTransmission(DS3231_ADDR);
    if (Wire.endTransmission() == 0) {
        if (!rtcFound_) {
            LOG(LogLevel::INFO, "RTC", "DS3231 RTC has been re-detected.");
        }
        rtcFound_ = true;
    } else {
        if (rtcFound_) {
            LOG(LogLevel::ERROR, "RTC", "DS3231 RTC communication failed.");
        }
        rtcFound_ = false;
    }
}

void RtcManager::setup(App* app) {
    app_ = app;
    checkForRtc();
    lastAttemptTime_ = millis();

    if (rtcFound_) {
        selectRtcMux();
        Wire.beginTransmission(DS3231_ADDR);
        Wire.write(0x0F);
        Wire.endTransmission();
        Wire.requestFrom(DS3231_ADDR, 1);
        uint8_t status = Wire.read();

        if (status & 0x80) {
            LOG(LogLevel::WARN, "RTC", "RTC lost power. Setting time to firmware compile time.");
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

            selectRtcMux();
            Wire.beginTransmission(DS3231_ADDR);
            Wire.write(0x0F);
            Wire.write(status & ~0x80);
            Wire.endTransmission();
        }
    } else {
        LOG(LogLevel::ERROR, "RTC", "DS3231 RTC not found on initial setup!");
    }
}

void RtcManager::update() {
    if (!rtcFound_ && (millis() - lastAttemptTime_ > RETRY_INTERVAL_MS)) {
        LOG(LogLevel::INFO, "RTC", "Attempting to reconnect to RTC...");
        checkForRtc();
        lastAttemptTime_ = millis();
    }
}

// --- NEW: The one-time synchronization function ---
void RtcManager::syncInternalClock() {
    if (!rtcFound_) {
        LOG(LogLevel::WARN, "RTC", "Cannot sync internal clock, RTC not found.");
        return;
    }

    LOG(LogLevel::INFO, "RTC", "Synchronizing ESP32 internal clock from DS3231.");
    GenericDateTime rtc_time = now();

    // If the read fails, now() will set rtcFound_ to false and return a zeroed struct.
    if (!rtcFound_) {
        LOG(LogLevel::ERROR, "RTC", "Failed to read from RTC during sync operation.");
        return;
    }

    struct tm timeinfo;
    timeinfo.tm_year = rtc_time.y - 1900;
    timeinfo.tm_mon  = rtc_time.m - 1;
    timeinfo.tm_mday = rtc_time.d;
    timeinfo.tm_hour = rtc_time.h;
    timeinfo.tm_min  = rtc_time.min;
    timeinfo.tm_sec  = rtc_time.s;

    time_t timestamp = mktime(&timeinfo);

    struct timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
}


bool RtcManager::isRtcFound() const {
    return rtcFound_;
}

GenericDateTime RtcManager::now() {
    GenericDateTime dt = {0,0,0,0,0,0};
    if (!rtcFound_) return dt;

    selectRtcMux();
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);
    
    if (Wire.endTransmission() != 0) {
        if (rtcFound_) { 
            LOG(LogLevel::ERROR, "RTC", "Failed to communicate with RTC (endTransmission). Marking as lost.");
        }
        rtcFound_ = false;
        lastAttemptTime_ = millis();
        return dt;
    }

    if (Wire.requestFrom(DS3231_ADDR, 7) != 7) {
        if (rtcFound_) { 
            LOG(LogLevel::ERROR, "RTC", "Failed to communicate with RTC (requestFrom). Marking as lost.");
        }
        rtcFound_ = false;
        lastAttemptTime_ = millis();
        return dt;
    }

    dt.s = bcdToDec(Wire.read() & 0x7F);
    dt.min = bcdToDec(Wire.read());
    dt.h = bcdToDec(Wire.read() & 0x3F);
    Wire.read();
    dt.d = bcdToDec(Wire.read());
    dt.m = bcdToDec(Wire.read() & 0x1F);
    dt.y = bcdToDec(Wire.read()) + 2000;

    return dt;
}

// --- MODIFIED: Reads from fast internal clock ---
std::string RtcManager::getFormattedTime() {
    time_t now_ts;
    time(&now_ts); // Get current timestamp from internal clock
    
    // If the timestamp is very small, it means the clock hasn't been set.
    if (now_ts < 1000000000) {
        return "--:--";
    }

    struct tm timeinfo;
    localtime_r(&now_ts, &timeinfo);

    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    return std::string(buf);
}

// --- MODIFIED: Reads from fast internal clock ---
std::string RtcManager::getFormattedDate() {
    time_t now_ts;
    time(&now_ts);
    if (now_ts < 1000000000) {
        return "----/--/--";
    }

    struct tm timeinfo;
    localtime_r(&now_ts, &timeinfo);

    char buf[11];
    snprintf(buf, sizeof(buf), "%04d/%02d/%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    return std::string(buf);
}

void RtcManager::onNtpSync() {
    // This function now correctly syncs the internal clock AND adjusts the external RTC.
    if (!rtcFound_ || app_->getWifiManager().getState() != WifiState::CONNECTED) return;
    if (millis() - lastNtpSyncTime_ < 600000) return;

    LOG(LogLevel::INFO, "RTC", "Attempting NTP time synchronization...");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10000)) {
        // Internal clock is now synced automatically by getLocalTime().
        
        // Now, update the external RTC with this new time.
        GenericDateTime ntp_dt = {
            (uint16_t)(timeinfo.tm_year + 1900),
            (uint8_t)(timeinfo.tm_mon + 1),
            (uint8_t)timeinfo.tm_mday,
            (uint8_t)timeinfo.tm_hour,
            (uint8_t)timeinfo.tm_min,
            (uint8_t)timeinfo.tm_sec
        };
        adjust(ntp_dt);
        LOG(LogLevel::INFO, "RTC", "NTP sync successful. RTC hardware updated.");
        lastNtpSyncTime_ = millis();
    } else {
        LOG(LogLevel::ERROR, "RTC", "NTP sync failed.");
    }
}