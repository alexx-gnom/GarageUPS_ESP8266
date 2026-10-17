#include "system_clock.h"

#include <ESP8266WiFi.h>

static constexpr time_t VALID_EPOCH = 1700000000; // 2023-11-14; rejects unset clock
static constexpr uint32_t NTP_RESYNC_INTERVAL_MS = 24UL * 60UL * 60UL * 1000UL;
static bool clockStarted = false;
static unsigned long lastSyncRequest = 0;
static bool wasValid = false;

void systemClockBegin() {
    // Kyiv local time with automatic EET/EEST daylight-saving transitions.
    

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("Clock: Wi-Fi unavailable; NTP will start when connected."));
        return;
    }

    configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");

    setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
    tzset();

    //sntp_update_delay(NTP_RESYNC_INTERVAL_MS);
    clockStarted = true;
    lastSyncRequest = millis();
    Serial.println(F("Clock: NTP configured; timezone Europe/Kyiv."));
}

void systemClockUpdate() {
    if (WiFi.status() != WL_CONNECTED) return;

    if (!clockStarted) {
        configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
        //sntp_update_delay(NTP_RESYNC_INTERVAL_MS);
        clockStarted = true;
        lastSyncRequest = millis();
        Serial.println(F("Clock: NTP started after Wi-Fi became available."));
    } else if (millis() - lastSyncRequest >= NTP_RESYNC_INTERVAL_MS) {
        // NTP's SNTP client also resynchronizes automatically; this timer keeps
        // our requested daily cadence explicit without blocking the main loop.
        configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
        lastSyncRequest = millis();
        Serial.println(F("Clock: daily NTP resync requested."));
    }

    const bool valid = systemClockIsValid();
    if (valid && !wasValid) {
        Serial.print(F("Clock: time synchronized: "));
        Serial.println(systemClockGetLocalTime());
    }
    wasValid = valid;
}

bool systemClockIsValid() {
    return time(nullptr) >= VALID_EPOCH;
}

String systemClockGetLocalTime() {
    if (!systemClockIsValid()) return String("Час ще не синхронізовано");

    time_t now = time(nullptr);
    struct tm localTimeInfo;
    localtime_r(&now, &localTimeInfo);

    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
             localTimeInfo.tm_year + 1900,
             localTimeInfo.tm_mon + 1,
             localTimeInfo.tm_mday,
             localTimeInfo.tm_hour,
             localTimeInfo.tm_min,
             localTimeInfo.tm_sec);
    return String(buffer);
}
