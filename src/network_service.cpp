#include "network_service.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>

#include "app_config.h"

void network_connect_wifi(AppState &state)
{
    Serial.println("\n[WiFi] Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(APP_WIFI_SSID, APP_WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        state.wifi_connected = true;
        Serial.println("\n[WiFi] Connected!");
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
    } else {
        state.wifi_connected = false;
        Serial.println("\n[WiFi] Connection failed!");
    }
}

bool network_is_connected(const AppState &state)
{
    return state.wifi_connected;
}

void network_sync_time()
{
    Serial.println("[NTP] Syncing time...");
    configTime(3600, 0, "pool.ntp.org", "time.nist.gov");

    tm timeinfo = {};
    int attempts = 0;
    while (!getLocalTime(&timeinfo) && attempts < 10) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (attempts < 10) {
        Serial.println("\n[NTP] Time synced!");
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("[NTP] Current time: %s\n", time_str);
    } else {
        Serial.println("\n[NTP] Time sync failed!");
    }
}

bool network_read_time(TimeSnapshot &snapshot)
{
    snapshot.valid = getLocalTime(&snapshot.local);
    return snapshot.valid;
}

void weather_update(AppState &state)
{
    if (!state.wifi_connected) return;

    HTTPClient http;
    String url = "https://api.openweathermap.org/data/3.0/onecall?lat=" + String(APP_WEATHER_LAT, 4) +
                 "&lon=" + String(APP_WEATHER_LON, 4) +
                 "&units=metric&exclude=minutely,hourly,daily,alerts&appid=" + String(APP_WEATHER_API_KEY);

    Serial.println("[Weather] Fetching weather...");
    http.begin(url);
    int http_code = http.GET();

    if (http_code == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            state.weather.temp_c = doc["current"]["temp"];
            state.weather.description = doc["current"]["weather"][0]["description"].as<String>();
            state.weather.valid = true;
            state.weather.updated_ms = millis();

            Serial.printf("[Weather] Temp: %.1fC, %s\n", state.weather.temp_c, state.weather.description.c_str());
        } else {
            Serial.printf("[Weather] JSON parse error: %s\n", error.c_str());
        }
    } else {
        Serial.printf("[Weather] HTTP error: %d\n", http_code);
        if (http_code == 401) {
            Serial.println("[Weather] API key might be invalid or inactive");
        }
    }

    http.end();
}

