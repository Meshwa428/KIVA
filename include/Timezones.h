#ifndef TIMEZONES_H
#define TIMEZONES_H

#include "TimezoneListDataSource.h"
#include <vector>

static const std::vector<TimezoneInfo> TIMEZONE_LIST = {
    // Americas
    {"UTC-11:00, Samoa", "Samoa", "SST11"},
    {"UTC-10:00, Hawaii", "Hawaii", "HST10"},
    {"UTC-09:00, Alaska", "Alaska", "AKST9AKDT,M3.2.0,M11.1.0"},
    {"UTC-08:00, Pacific", "US Pacific", "PST8PDT,M3.2.0,M11.1.0"},
    {"UTC-07:00, Mountain", "US Mountain", "MST7MDT,M3.2.0,M11.1.0"},
    {"UTC-07:00, Arizona", "Arizona", "MST7"},
    {"UTC-06:00, Central", "US Central", "CST6CDT,M3.2.0,M11.1.0"},
    {"UTC-05:00, Eastern", "US Eastern", "EST5EDT,M3.2.0,M11.1.0"},
    {"UTC-04:00, Atlantic", "Atlantic", "AST4ADT,M3.2.0,M11.1.0"},
    {"UTC-03:00, Brazil", "Brazil", "BRT3BRST,M10.3.0,M2.3.0"},
    {"UTC-03:30, Newfoundland", "Newfoundland", "NST3:30NDT,M3.2.0,M11.1.0"},

    // Europe & Africa
    {"UTC+00:00, London", "London", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"UTC+01:00, Berlin", "Berlin", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"UTC+02:00, Athens", "Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"UTC+03:00, Moscow", "Moscow", "MSK-3"},

    // Asia & Middle East
    {"UTC+03:30, Tehran", "Tehran", "IRST-3:30"},
    {"UTC+04:00, Dubai", "Dubai", "GST-4"},
    {"UTC+04:30, Kabul", "Kabul", "AFT-4:30"},
    {"UTC+05:00, Karachi", "Karachi", "PKT-5"},
    {"UTC+05:30, India", "India", "IST-5:30"},
    {"UTC+05:45, Nepal", "Nepal", "NPT-5:45"},
    {"UTC+06:00, Dhaka", "Dhaka", "BST-6"},
    {"UTC+07:00, Bangkok", "Bangkok", "ICT-7"},
    {"UTC+08:00, Hong Kong", "Hong Kong", "HKT-8"},
    {"UTC+08:00, Perth", "Perth", "AWST-8"},
    {"UTC+09:00, Tokyo", "Tokyo", "JST-9"},
    {"UTC+09:30, Adelaide", "Adelaide", "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
    {"UTC+09:30, Darwin", "Darwin", "ACST-9:30"},

    // Australia & Pacific
    {"UTC+10:00, Sydney", "Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"UTC+11:00, Solomon Is.", "Solomon Is.", "SBT-11"},
    {"UTC+12:00, Auckland", "Auckland", "NZST-12NZDT,M9.5.0,M4.1.0/3"},

    // Universal
    {"UTC", "UTC", "UTC0"},
};

#endif // TIMEZONES_H
