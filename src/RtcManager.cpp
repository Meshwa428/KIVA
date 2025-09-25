#include "RtcManager.h"
#include "App.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "Config.h"
#include <Wire.h>
#include <sys/time.h> 
#include <time.h> 
#include "esp_sntp.h"

#define DS3231_ADDR 0x68

RtcManager::RtcManager() : app_(nullptr), rtcFound_(false), lastNtpSyncTime_(0), lastAttemptTime_(0), syncPending_(false) {}

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
    Wire.write(decToBcd(dt.dow));
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
        // 1. First, sync the ESP32's internal clock from the hardware RTC's UTC time.
        syncInternalClock();

        // 2. Now that the internal clock has a valid base time, apply the timezone.
        const char* tz_string = app_->getConfigManager().getSettings().timezoneString;
        setenv("TZ", tz_string, 1);
        tzset();
        LOG(LogLevel::INFO, "RTC", "Applied timezone from settings: %s", tz_string);
        
        // 3. Check if the RTC lost power and needs to be reset to compile time.
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
            
            struct tm timeinfo_compile;
            timeinfo_compile.tm_year = compiled_dt.y - 1900;
            timeinfo_compile.tm_mon = compiled_dt.m - 1;
            timeinfo_compile.tm_mday = compiled_dt.d;
            timeinfo_compile.tm_hour = compiled_dt.h;
            timeinfo_compile.tm_min = compiled_dt.min;
            timeinfo_compile.tm_sec = compiled_dt.s;
            timeinfo_compile.tm_isdst = -1;
            mktime(&timeinfo_compile); 
            compiled_dt.dow = timeinfo_compile.tm_wday + 1; 

            adjust(compiled_dt);

            // After adjusting, re-sync the internal clock to be sure.
            syncInternalClock();

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

void RtcManager::setTimezone(const char* tzString) {
    setenv("TZ", tzString, 1);
    tzset();
    LOG(LogLevel::INFO, "RTC_UPDATE", "Timezone set to '%s' for display.", tzString);
}


void RtcManager::update() {
    if (syncPending_) {
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            syncPending_ = false;

            LOG(LogLevel::INFO, "RTC_UPDATE", "SNTP status is COMPLETED. Finalizing sync.");

            struct tm utc_timeinfo;
            if (!getLocalTime(&utc_timeinfo)) {
                LOG(LogLevel::ERROR, "RTC_UPDATE", "getLocalTime() failed even after sync completion.");
                return;
            }

            char utc_time_str[25];
            strftime(utc_time_str, sizeof(utc_time_str), "%Y-%m-%d %H:%M:%S", &utc_timeinfo);
            LOG(LogLevel::INFO, "RTC_UPDATE", "NTP sync complete. Correct UTC time is: %s", utc_time_str);

            const char* tz_string = app_->getConfigManager().getSettings().timezoneString;
            setenv("TZ", tz_string, 1);
            tzset();
            LOG(LogLevel::INFO, "RTC_UPDATE", "Timezone set to '%s' for display.", tz_string);

            if (rtcFound_) {
                GenericDateTime time_before = now();
                char before_str[25];
                snprintf(before_str, sizeof(before_str), "%04d-%02d-%02d %02d:%02d:%02d",
                         time_before.y, time_before.m, time_before.d,
                         time_before.h, time_before.min, time_before.s);
                LOG(LogLevel::INFO, "RTC_UPDATE", "Time on DS3231 before final sync: %s", before_str);

                GenericDateTime utc_dt_to_write = {
                    (uint16_t)(utc_timeinfo.tm_year + 1900), (uint8_t)(utc_timeinfo.tm_mon + 1),
                    (uint8_t)utc_timeinfo.tm_mday, (uint8_t)utc_timeinfo.tm_hour,
                    (uint8_t)utc_timeinfo.tm_min, (uint8_t)utc_timeinfo.tm_sec,
                    (uint8_t)(utc_timeinfo.tm_wday + 1)
                };
                
                adjust(utc_dt_to_write);
                LOG(LogLevel::INFO, "RTC_UPDATE", "Writing final UTC time to DS3231...");

                GenericDateTime time_after = now();
                char after_str[25];
                snprintf(after_str, sizeof(after_str), "%04d-%02d-%02d %02d:%02d:%02d",
                         time_after.y, time_after.m, time_after.d,
                         time_after.h, time_after.min, time_after.s);
                LOG(LogLevel::INFO, "RTC_UPDATE", "Time on DS3231 after final sync:  %s", after_str);
            }
            lastNtpSyncTime_ = millis();
        }
    }

    if (!rtcFound_ && (millis() - lastAttemptTime_ > RETRY_INTERVAL_MS)) {
        LOG(LogLevel::INFO, "RTC", "Attempting to reconnect to RTC...");
        checkForRtc();
        lastAttemptTime_ = millis();
    }
}

void RtcManager::syncInternalClock() {
    if (!rtcFound_) {
        LOG(LogLevel::WARN, "RTC", "Cannot sync internal clock, RTC not found.");
        return;
    }

    LOG(LogLevel::INFO, "RTC", "Synchronizing ESP32 internal clock from DS3231.");
    GenericDateTime rtc_time = now();

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
    timeinfo.tm_isdst = 0;

    char* old_tz = getenv("TZ");
    setenv("TZ", "UTC0", 1);
    tzset();

    time_t timestamp = mktime(&timeinfo);

    if (old_tz) {
        setenv("TZ", old_tz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();

    struct timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
}

bool RtcManager::isRtcFound() const { return rtcFound_; }

GenericDateTime RtcManager::now() {
    GenericDateTime dt = {0,0,0,0,0,0,0};
    if (!rtcFound_) return dt;

    selectRtcMux();
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);
    
    if (Wire.endTransmission() != 0) {
        if (rtcFound_) { LOG(LogLevel::ERROR, "RTC", "Failed to communicate with RTC (endTransmission). Marking as lost."); }
        rtcFound_ = false;
        lastAttemptTime_ = millis();
        return dt;
    }

    if (Wire.requestFrom(DS3231_ADDR, 7) != 7) {
        if (rtcFound_) { LOG(LogLevel::ERROR, "RTC", "Failed to communicate with RTC (requestFrom). Marking as lost."); }
        rtcFound_ = false;
        lastAttemptTime_ = millis();
        return dt;
    }

    dt.s = bcdToDec(Wire.read() & 0x7F);
    dt.min = bcdToDec(Wire.read());
    dt.h = bcdToDec(Wire.read() & 0x3F);
    dt.dow = bcdToDec(Wire.read() & 0x07);
    dt.d = bcdToDec(Wire.read());
    dt.m = bcdToDec(Wire.read() & 0x1F);
    dt.y = bcdToDec(Wire.read()) + 2000;

    return dt;
}

std::string RtcManager::getFormattedTime() {
    time_t now_ts;
    time(&now_ts); 
    if (now_ts < 1000000000) { return "--:--"; }
    struct tm timeinfo;
    localtime_r(&now_ts, &timeinfo);
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    return std::string(buf);
}

std::string RtcManager::getFormattedDate() {
    time_t now_ts;
    time(&now_ts);
    if (now_ts < 1000000000) { return "----/--/--"; }
    struct tm timeinfo;
    localtime_r(&now_ts, &timeinfo);
    char buf[11];
    snprintf(buf, sizeof(buf), "%04d/%02d/%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    return std::string(buf);
}

void RtcManager::onNtpSync() {
    if (app_->getWifiManager().getState() != WifiState::CONNECTED) return;
    if (lastNtpSyncTime_ != 0 && millis() - lastNtpSyncTime_ < 600000) return;

    LOG(LogLevel::INFO, "RTC_CB", "Starting NTP time synchronization process...");
    
    // --- MODIFICATION START: Revert to simple UTC config ---
    // This tells the SNTP client to get pure UTC time and set the internal
    // clock to it, without any timezone meddling at this stage.
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    // --- MODIFICATION END ---
    
    syncPending_ = true;
}