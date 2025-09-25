// KIVA/include/RtcManager.h

#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include "GenericDateTime.h"
#include <string>

class App;

#include "Service.h"

class RtcManager : public Service {
public:
    RtcManager();
    void setup(App* app) override;
    void update();

    void syncInternalClock();
    bool isRtcFound() const;
    GenericDateTime now();
    std::string getFormattedTime();
    std::string getFormattedDate();
    void onNtpSync();
    void setTimezone(const char* tzString);

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

    volatile bool syncPending_;

    static constexpr unsigned long RETRY_INTERVAL_MS = 10000;
};

#endif // RTC_MANAGER_H