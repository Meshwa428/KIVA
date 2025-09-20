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

    bool isRtcFound() const;
    GenericDateTime now();
    std::string getFormattedTime();
    std::string getFormattedDate();

    void onNtpSync();

private:
    void selectRtcMux();
    void adjust(const GenericDateTime& dt);
    static uint8_t bcdToDec(uint8_t val);
    static uint8_t decToBcd(uint8_t val);

    App* app_;
    bool rtcFound_;
    unsigned long lastNtpSyncTime_;
};

#endif // RTC_MANAGER_H
