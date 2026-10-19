#include "telegram.h"

#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "secrets.h"
#include "sensors.h"
#include "outputs.h"
#include "protection.h"
#include "system_clock.h"

static constexpr char TELEGRAM_HOST[] = "api.telegram.org";
static constexpr uint16_t TELEGRAM_PORT = 443;
static constexpr uint8_t TELEGRAM_QUEUE_CAPACITY = 5;
static constexpr size_t TELEGRAM_MESSAGE_SIZE = 640;
static constexpr unsigned long TELEGRAM_SUCCESS_GAP_MS = 5000UL;
static constexpr unsigned long TELEGRAM_RETRY_GAP_MS = 60000UL;

static char messageQueue[TELEGRAM_QUEUE_CAPACITY][TELEGRAM_MESSAGE_SIZE];
static uint8_t queueHead = 0;
static uint8_t queueTail = 0;
static uint8_t queueCount = 0;
static bool attemptedSend = false;
static unsigned long lastSendAttempt = 0;
static unsigned long nextSendDelay = TELEGRAM_SUCCESS_GAP_MS;
static bool stateSnapshotReady = false;

struct ObservedState {
    bool sourcePresent;
    bool lowBattery;
    bool overheat;
    bool temperatureSensorsOk;
};
static ObservedState lastObservedState = { false, false, false, false };
static int lastDailyReportYear = -1;
static int lastDailyReportYday = -1;

bool telegramIsConfigured() {
    return TELEGRAM_BOT_TOKEN != nullptr && TELEGRAM_CHAT_ID != nullptr &&
           TELEGRAM_BOT_TOKEN[0] != '\0' && TELEGRAM_CHAT_ID[0] != '\0' &&
           strcmp(TELEGRAM_BOT_TOKEN, "PASTE_BOT_TOKEN_HERE") != 0 /*&&
           strcmp(TELEGRAM_CHAT_ID, "PASTE_CHAT_ID_HERE") != 0*/;
}

static bool enqueueMessage(const char* text) {
    if (!text || !*text) return false;
    if (queueCount >= TELEGRAM_QUEUE_CAPACITY) {
        Serial.println(F("Telegram: queue full; message not queued."));
        return false;
    }

    strncpy(messageQueue[queueTail], text, TELEGRAM_MESSAGE_SIZE - 1);
    messageQueue[queueTail][TELEGRAM_MESSAGE_SIZE - 1] = '\0';
    queueTail = static_cast<uint8_t>((queueTail + 1) % TELEGRAM_QUEUE_CAPACITY);
    ++queueCount;
    return true;
}

static String formatFloat(float value, uint8_t decimals) {
    if (!isfinite(value)) return String("н/д");
    return String(value, decimals);
}

static String formatUptime() {
    const uint32_t total = millis() / 1000UL;
    const uint32_t days = total / 86400UL;
    const uint8_t hours = (total % 86400UL) / 3600UL;
    const uint8_t minutes = (total % 3600UL) / 60UL;
    const uint8_t seconds = total % 60UL;
    char buffer[40];
    if (days > 0) {
        snprintf(buffer, sizeof(buffer), "%lu д %02u:%02u:%02u",
                 static_cast<unsigned long>(days), hours, minutes, seconds);
    } else {
        snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", hours, minutes, seconds);
    }
    return String(buffer);
}

static String buildStatusReport(const char* source) {
    String text;
    text.reserve(620);
    text += F("🔋 Gn.O.M Garage UPS — звіт\n");
    text += F("Запит: ");
    text += (source && *source) ? source : "ручний";
    text += '\n';
    text += F("Час: ");
    text += systemClockGetLocalTime();
    text += F(" (Europe/Kyiv)\n\n");

    text += F("Блок живдення: ");
    text += formatFloat(sensorsChargerVoltage(), 2);
    text += F(" V \nМережа:  ");
    text += protectionIsChargerPresent() ? F("є") : F("немає");
    text += '\n';
    text += F("Акумулятор: ");
    text += formatFloat(sensorsBatteryVoltage(), 2);
    text += F(" V\nСтрум заряду: ");
    text += formatFloat(sensorsCurrentAmps(), 2);
    text += F(" A\nТемпература АКБ: ");
    text += formatFloat(sensorsNtcTemperature(), 1);
    text += F(" °C\nЗовнішня температура: ");
    text += formatFloat(sensorsDs18b20Temperature(), 2);
    text += F(" °C\n\n");

    text += F("Термозахист: ");
    text += protectionIsOverheatLatched() ? F("АВАРІЯ") : F("норма");
    text += '\n';
    text += F("Датчики температури: ");
    text += protectionTemperatureSensorsOk() ? F("OK") : F("ПОМИЛКА");
    text += '\n';
    text += F("Низька напруга АКБ: ");
    text += protectionIsLowBatteryCutoff() ? F("ТАК!") : F("ні");
    text += '\n';
    text += F("Зарядний вихід: ");
    text += outputsGetSafety() ? F("ON") : F("OFF");
    text += F(" (запит ");
    text += protectionGetChargerRequest() ? F("ON") : F("OFF");
    text += F(")\n");
    text += F("LOAD 1: ");
    text += outputsGetLoad0() ? F("ON") : F("OFF");
    text += F(" (запит ");
    text += protectionGetLoad0Desired() ? F("ON") : F("OFF");
    text += F(")\nLOAD 2: ");
    text += outputsGetLoad1() ? F("ON") : F("OFF");
    text += F(" (запит ");
    text += protectionGetLoad1Desired() ? F("ON") : F("OFF");
    text += F(")\nUptime: ");
    text += formatUptime();
    return text;
}

bool telegramRequestStatusReport(const char* source) {
    if (!telegramIsConfigured()) {
        Serial.println(F("Telegram: not configured; add bot token and chat ID in secrets.cpp."));
        return false;
    }
    const String report = buildStatusReport(source);
    const bool queued = enqueueMessage(report.c_str());
    if (queued) Serial.println(F("Telegram: status report queued."));
    return queued;
}

static String jsonEscape(const char* input) {
    String out;
    if (!input) return out;
    out.reserve(strlen(input) + 24);
    for (const char* p = input; *p; ++p) {
        const char c = *p;
        switch (c) {
            case '\\': out += F("\\\\"); break;
            case '"':  out += F("\\\""); break;
            case '\n': out += F("\\n"); break;
            case '\r': out += F("\\r"); break;
            case '\t': out += F("\\t"); break;
            default:
                if (static_cast<uint8_t>(c) >= 0x20) out += c;
                break;
        }
    }
    return out;
}

static bool sendTelegramMessage(const char* message) {
    if (!telegramIsConfigured() || WiFi.status() != WL_CONNECTED) return false;

    BearSSL::WiFiClientSecure client;
    // TLS is encrypted, but the peer certificate is not validated in this first version.
    // Keep timeouts short to reduce the loop delay if the Telegram endpoint is unavailable.
    client.setInsecure();
    client.setTimeout(1200);
    //client.setHandshakeTimeout(2);

    Serial.println(F("Telegram: connecting to Bot API..."));
    const unsigned long started = millis();
    if (!client.connect(TELEGRAM_HOST, TELEGRAM_PORT)) {
        Serial.printf("Telegram: connect failed after %lu ms.\n",
                      static_cast<unsigned long>(millis() - started));
        client.stop();
        return false;
    }

    String escapedText = jsonEscape(message);
    String payload;
    payload.reserve(escapedText.length() + TELEGRAM_CHAT_ID.length() + 40);
    payload = F("{\"chat_id\":\"");
    payload += TELEGRAM_CHAT_ID;
    payload += F("\",\"text\":\"");
    payload += escapedText;
    payload += F("\"}");

    client.print(F("POST /bot"));
    client.print(TELEGRAM_BOT_TOKEN);
    client.println(F("/sendMessage HTTP/1.1"));
    client.println(F("Host: api.telegram.org"));
    client.println(F("Connection: close"));
    client.println(F("Content-Type: application/json; charset=utf-8"));
    client.print(F("Content-Length: "));
    client.println(payload.length());
    client.println();
    client.print(payload);

    const unsigned long responseStarted = millis();
    while (client.connected() && !client.available() &&
           millis() - responseStarted < 1800UL) {
        delay(1);
    }

    String statusLine;
    if (client.available()) {
        statusLine = client.readStringUntil('\n');
        statusLine.trim();
    }
    client.stop();

    const unsigned long elapsed = millis() - started;
    const bool ok = statusLine.startsWith("HTTP/1.1 200") ||
                    statusLine.startsWith("HTTP/1.0 200");
    Serial.printf("Telegram: send %s after %lu ms; HTTP: %s\n",
                  ok ? "OK" : "FAILED", static_cast<unsigned long>(elapsed),
                  statusLine.length() ? statusLine.c_str() : "no response");
    return ok;
}

static void queueAlert(const String& message) {
    if (!telegramIsConfigured()) return;
    enqueueMessage(message.c_str());
}

static String withCurrentTime(const char* heading) {
    String message;
    message.reserve(220);
    message += heading;
    message += '\n';
    message += F("Час: ");
    message += systemClockGetLocalTime();
    message += F(" (Europe/Kyiv)\nАкумулятор: ");
    message += formatFloat(sensorsBatteryVoltage(), 2);
    message += F(" V\nБлок живлення: ");
    message += protectionIsChargerPresent() ? F("є") : F("немає");
    message += F(" (");
    message += formatFloat(sensorsChargerVoltage(), 2);
    message += F(" V)");
    return message;
}

static void checkStateTransitions() {
    const ObservedState current = {
        protectionIsChargerPresent(),
        protectionIsLowBatteryCutoff(),
        protectionIsOverheatLatched(),
        protectionTemperatureSensorsOk()
    };

    if (!stateSnapshotReady) {
        lastObservedState = current;
        stateSnapshotReady = true;

        // Report any critical protection condition already active at boot once.
        String startupWarnings;
        if (current.overheat || !current.temperatureSensorsOk || current.lowBattery) {
            startupWarnings = withCurrentTime("⚠️ UPS запустилась з активним захистом:");
            if (current.overheat) startupWarnings += F("\n• Температурна аварія.");
            if (!current.temperatureSensorsOk) startupWarnings += F("\n• Датчики температури несправні або ще не готові.");
            if (current.lowBattery) startupWarnings += F("\n• Активний захист акумулятора по низькій напрузі.");
            queueAlert(startupWarnings);
        }
        return;
    }

    if (lastObservedState.sourcePresent && !current.sourcePresent) {
        queueAlert(withCurrentTime("⚠️ Живлення зникло!"));
    } else if (!lastObservedState.sourcePresent && current.sourcePresent) {
        queueAlert(withCurrentTime("✅ Живлення відновлено."));
    }

    if (!lastObservedState.lowBattery && current.lowBattery) {
        String message = withCurrentTime("🚨 Низька напруга АКБ: Виходи вимкнені.");
        message += F("\nLOAD 1: ");
        message += outputsGetLoad0() ? F("ON") : F("OFF");
        message += F("; LOAD 2: ");
        message += outputsGetLoad1() ? F("ON") : F("OFF");
        queueAlert(message);
    } else if (lastObservedState.lowBattery && !current.lowBattery) {
        queueAlert(withCurrentTime("✅ Захист акумулятора по низькій напрузі знято."));
    }

    if (!lastObservedState.overheat && current.overheat) {
        queueAlert(withCurrentTime("🚨 ТЕМПЕРАТУРНА АВАРІЯ! Виходи вимкнені."));
    } else if (lastObservedState.overheat && !current.overheat) {
        queueAlert(withCurrentTime("✅ Температурну аварію скинуто."));
    }

    if (lastObservedState.temperatureSensorsOk && !current.temperatureSensorsOk) {
        queueAlert(withCurrentTime("🚨 Помилка датчика температури! Виходи вимкнені."));
    } else if (!lastObservedState.temperatureSensorsOk && current.temperatureSensorsOk) {
        queueAlert(withCurrentTime("✅ Датчики температури знову працюють."));
    }

    lastObservedState = current;
}

static void checkDailyReport() {
    if (!systemClockIsValid() || !telegramIsConfigured()) return;

    const time_t now = time(nullptr);
    struct tm localInfo;
    if (!localtime_r(&now, &localInfo)) return;
    if (localInfo.tm_hour != 8) return;

    const int year = localInfo.tm_year + 1900;
    if (lastDailyReportYear == year && lastDailyReportYday == localInfo.tm_yday) return;

    if (telegramRequestStatusReport("За розкладом")) {
        lastDailyReportYear = year;
        lastDailyReportYday = localInfo.tm_yday;
    }
}

void telegramBegin() {
    if (telegramIsConfigured()) {
        Serial.println(F("Telegram: initialized."));
    } else {
        Serial.println(F("Telegram: not configured; status messages are disabled until secrets.cpp is updated."));
    }
}

void telegramUpdate() {
    checkStateTransitions();
    checkDailyReport();

    if (queueCount == 0 || WiFi.status() != WL_CONNECTED || !telegramIsConfigured()) return;

    const unsigned long now = millis();
    if (attemptedSend && now - lastSendAttempt < nextSendDelay) return;
    attemptedSend = true;
    lastSendAttempt = now;

    if (sendTelegramMessage(messageQueue[queueHead])) {
        queueHead = static_cast<uint8_t>((queueHead + 1) % TELEGRAM_QUEUE_CAPACITY);
        --queueCount;
        nextSendDelay = TELEGRAM_SUCCESS_GAP_MS;
    } else {
        nextSendDelay = TELEGRAM_RETRY_GAP_MS;
    }
}
