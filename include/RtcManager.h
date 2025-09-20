#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include "GenericDateTime.h"
#include <string>

class App;

class RtcManager {
public:
    RtcManager();
    void setup(App* app);
    void update();

    // --- NEW: Function to perform the one-time sync ---
    void syncInternalClock();

    bool isRtcFound() const;
    GenericDateTime now();
    std::string getFormattedTime();
    std::string getFormattedDate();

    void onNtpSync();

private:
    void selectRtcMux();
    void adjust(const GenericDateTime& dt);
    void checkForRtc(); 
    static uint8_t bcdToDec(uint8_t val);
    static uint8_t decToBcd(uint8_t val);

    App* app_;
    bool rtcFound_;
    unsigned long lastNtpSyncTime_;
    unsigned long lastAttemptTime_;
    static constexpr unsigned long RETRY_INTERVAL_MS = 10000;
};

#endif // RTC_MANAGER_H