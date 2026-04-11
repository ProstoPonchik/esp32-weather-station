#include "components/network/network_service.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <time.h>

#include "core/app_config.h"

static String normalize_icon_code(const String &icon_code)
{
    if (icon_code.length() < 3) return "03d";

    String prefix = icon_code.substring(0, 2);
    char day_night = icon_code.charAt(2);
    if (day_night != 'd' && day_night != 'n') day_night = 'd';

    if (prefix == "01" || prefix == "02" || prefix == "10") return prefix + String(day_night);
    if (prefix == "03" || prefix == "04" || prefix == "09" || prefix == "11" ||
        prefix == "13" || prefix == "50") return prefix + "d";

    return "03d";
}

static String format_time_hhmm(uint32_t unix_utc, int32_t timezone_offset_sec)
{
    time_t ts = static_cast<time_t>(unix_utc + timezone_offset_sec);
    tm tm_info = {};
    gmtime_r(&ts, &tm_info);

    char buf[8];
    strftime(buf, sizeof(buf), "%H:%M", &tm_info);
    return String(buf);
}

static String format_day_short(uint32_t unix_utc, int32_t timezone_offset_sec)
{
    time_t ts = static_cast<time_t>(unix_utc + timezone_offset_sec);
    tm tm_info = {};
    gmtime_r(&ts, &tm_info);

    char buf[8];
    strftime(buf, sizeof(buf), "%a", &tm_info);
    return String(buf);
}

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
    if (!state.wifi_connected) {
        Serial.println("[Weather][DBG] Skip update: WiFi disconnected");
        return;
    }

    HTTPClient http;
    WiFiClientSecure secure_client;
    String url = "https://api.openweathermap.org/data/3.0/onecall?lat=" + String(APP_WEATHER_LAT, 4) +
                 "&lon=" + String(APP_WEATHER_LON, 4) +
                 "&units=metric&exclude=minutely,alerts&appid=" + String(APP_WEATHER_API_KEY);

    Serial.println("[Weather] Fetching weather...");
    Serial.printf("[Weather][DBG] URL: %s\n", url.c_str());
    Serial.printf("[Weather][DBG] Heap before HTTPS: free=%u min=%u largest=%u largest_internal=%u free_internal=%u\n",
                  static_cast<unsigned>(ESP.getFreeHeap()),
                  static_cast<unsigned>(ESP.getMinFreeHeap()),
                  static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)),
                  static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                  static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));

    // Reduce TLS overhead (certificate verification can be expensive on constrained RAM).
    secure_client.setInsecure();
    secure_client.setTimeout(15000);
    http.setConnectTimeout(15000);
    http.setTimeout(15000);
    http.setReuse(false);

    if (!http.begin(secure_client, url)) {
        Serial.println("[Weather] ERROR: http.begin failed");
        return;
    }
    int http_code = http.GET();
    Serial.printf("[Weather][DBG] HTTP code: %d\n", http_code);
    Serial.printf("[Weather][DBG] Heap after GET: free=%u min=%u largest=%u largest_internal=%u free_internal=%u\n",
                  static_cast<unsigned>(ESP.getFreeHeap()),
                  static_cast<unsigned>(ESP.getMinFreeHeap()),
                  static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)),
                  static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                  static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));

    if (http_code == 200) {
        int content_len = http.getSize();
        Serial.printf("[Weather][DBG] Payload size(header): %d bytes\n", content_len);
        // Filter now includes hourly[0..7] and daily[0..7], so keep enough headroom.
        DynamicJsonDocument filter(4096);
        filter["timezone_offset"] = true;
        filter["current"]["temp"] = true;
        filter["current"]["pressure"] = true;
        filter["current"]["uvi"] = true;
        filter["current"]["dt"] = true;
        filter["current"]["sunrise"] = true;
        filter["current"]["sunset"] = true;
        filter["current"]["weather"][0]["id"] = true;
        filter["current"]["weather"][0]["icon"] = true;
        filter["current"]["weather"][0]["description"] = true;
        for (int i = 0; i < 8; i++) {
            filter["hourly"][i]["dt"] = true;
            filter["hourly"][i]["temp"] = true;
            filter["hourly"][i]["weather"][0]["icon"] = true;
        }

        for (int i = 0; i < 8; i++) {
            filter["daily"][i]["dt"] = true;
            filter["daily"][i]["temp"]["min"] = true;
            filter["daily"][i]["temp"]["max"] = true;
            filter["daily"][i]["weather"][0]["icon"] = true;
        }

        if (filter.overflowed()) {
            Serial.println("[Weather] ERROR: JSON filter document overflow");
        }
        Serial.printf("[Weather][DBG] Filter mem usage: %u / %u\n",
                      static_cast<unsigned>(filter.memoryUsage()),
                      static_cast<unsigned>(filter.capacity()));

        DynamicJsonDocument doc(16384);
        DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
        Serial.printf("[Weather][DBG] Deserialize status: %s\n", error ? error.c_str() : "ok");
        Serial.printf("[Weather][DBG] Doc mem usage: %u / %u\n",
                      static_cast<unsigned>(doc.memoryUsage()),
                      static_cast<unsigned>(doc.capacity()));

        if (!error) {
            if (doc.overflowed()) {
                Serial.println("[Weather] ERROR: JSON payload document overflow");
            }
            Serial.printf("[Weather][DBG] Field present: temp=%d pressure=%d uvi=%d sunrise=%d sunset=%d icon=%d desc=%d\n",
                          doc["current"]["temp"].isNull() ? 0 : 1,
                          doc["current"]["pressure"].isNull() ? 0 : 1,
                          doc["current"]["uvi"].isNull() ? 0 : 1,
                          doc["current"]["sunrise"].isNull() ? 0 : 1,
                          doc["current"]["sunset"].isNull() ? 0 : 1,
                          doc["current"]["weather"][0]["icon"].isNull() ? 0 : 1,
                          doc["current"]["weather"][0]["description"].isNull() ? 0 : 1);

            int32_t timezone_offset = doc["timezone_offset"] | 0;
            uint32_t api_now_utc = doc["current"]["dt"] | 0;
            if (api_now_utc == 0) {
                api_now_utc = static_cast<uint32_t>(time(nullptr));
            }

            state.weather.temp_c = doc["current"]["temp"];
            state.weather.pressure_hpa = doc["current"]["pressure"] | 0.0f;
            state.weather.uvi = doc["current"]["uvi"] | -1.0f;
            state.weather.description = doc["current"]["weather"][0]["description"].as<String>();
            state.weather.icon_code = normalize_icon_code(doc["current"]["weather"][0]["icon"].as<String>());
            state.weather.sunrise = format_time_hhmm(doc["current"]["sunrise"] | 0, timezone_offset);
            state.weather.sunset = format_time_hhmm(doc["current"]["sunset"] | 0, timezone_offset);
            Serial.printf("[Weather][DBG] Parsed current: temp=%.2f pressure=%.1f uvi=%.2f icon=%s desc=%s\n",
                          state.weather.temp_c,
                          state.weather.pressure_hpa,
                          state.weather.uvi,
                          state.weather.icon_code.c_str(),
                          state.weather.description.c_str());

            for (int i = 0; i < 3; i++) state.weather.hourly[i].valid = false;

            // Use API current.dt as a single source of truth, independent from MCU clock drift.
            uint32_t min_future_dt = api_now_utc + 1;

            int out_idx = 0;
            JsonArray hourly_arr = doc["hourly"].as<JsonArray>();
            for (JsonVariantConst item : hourly_arr) {
                if (out_idx >= 3) break;

                uint32_t dt = item["dt"] | 0;
                if (dt == 0 || dt < min_future_dt) continue;

                state.weather.hourly[out_idx].time_label = format_time_hhmm(dt, timezone_offset);
                state.weather.hourly[out_idx].temp_c = item["temp"] | 0.0f;
                state.weather.hourly[out_idx].icon_code =
                    normalize_icon_code(item["weather"][0]["icon"].as<String>());
                state.weather.hourly[out_idx].valid = true;
                Serial.printf("[Weather] Hourly[%d] %s %.1fC\n",
                              out_idx,
                              state.weather.hourly[out_idx].time_label.c_str(),
                              state.weather.hourly[out_idx].temp_c);
                out_idx++;
            }

            // Fallback: keep selecting only future slots if filtered payload was too short.
            for (int i = 0; i < 3 && out_idx < 3; i++) {
                uint32_t dt = doc["hourly"][i]["dt"] | 0;
                if (dt == 0 || dt <= api_now_utc) continue;

                state.weather.hourly[out_idx].time_label = format_time_hhmm(dt, timezone_offset);
                state.weather.hourly[out_idx].temp_c = doc["hourly"][i]["temp"] | 0.0f;
                state.weather.hourly[out_idx].icon_code =
                    normalize_icon_code(doc["hourly"][i]["weather"][0]["icon"].as<String>());
                state.weather.hourly[out_idx].valid = true;
                Serial.printf("[Weather] Hourly[%d] %s %.1fC (fallback)\n",
                              out_idx,
                              state.weather.hourly[out_idx].time_label.c_str(),
                              state.weather.hourly[out_idx].temp_c);
                out_idx++;
            }

            for (int i = 0; i < 3; i++) state.weather.daily[i].valid = false;

            int32_t now_day_key = static_cast<int32_t>((static_cast<int64_t>(api_now_utc) + timezone_offset) / 86400);

            int daily_out_idx = 0;
            JsonArray daily_arr = doc["daily"].as<JsonArray>();
            for (JsonVariantConst item : daily_arr) {
                if (daily_out_idx >= 3) break;

                uint32_t dt = item["dt"] | 0;
                if (dt == 0) continue;
                int32_t day_key = static_cast<int32_t>((static_cast<int64_t>(dt) + timezone_offset) / 86400);
                if (day_key <= now_day_key) continue; // skip today and any past bucket

                if (item["temp"]["min"].isNull() || item["temp"]["max"].isNull()) continue;

                float tmin = item["temp"]["min"] | 0.0f;
                float tmax = item["temp"]["max"] | 0.0f;

                state.weather.daily[daily_out_idx].day_label = format_day_short(dt, timezone_offset);
                state.weather.daily[daily_out_idx].min_c = tmin;
                state.weather.daily[daily_out_idx].max_c = tmax;
                state.weather.daily[daily_out_idx].icon_code =
                    normalize_icon_code(item["weather"][0]["icon"].as<String>());
                state.weather.daily[daily_out_idx].valid = true;
                Serial.printf("[Weather] Daily[%d] %s max=%.1f min=%.1f\n",
                              daily_out_idx,
                              state.weather.daily[daily_out_idx].day_label.c_str(),
                              state.weather.daily[daily_out_idx].max_c,
                              state.weather.daily[daily_out_idx].min_c);
                daily_out_idx++;
            }

            // Fallback: still do not allow today's bucket.
            for (int i = 0; i < 3 && daily_out_idx < 3; i++) {
                uint32_t dt = doc["daily"][i]["dt"] | 0;
                if (dt == 0) continue;
                int32_t day_key = static_cast<int32_t>((static_cast<int64_t>(dt) + timezone_offset) / 86400);
                if (day_key <= now_day_key) continue;

                state.weather.daily[daily_out_idx].day_label = format_day_short(dt, timezone_offset);
                state.weather.daily[daily_out_idx].min_c = doc["daily"][i]["temp"]["min"] | 0.0f;
                state.weather.daily[daily_out_idx].max_c = doc["daily"][i]["temp"]["max"] | 0.0f;
                state.weather.daily[daily_out_idx].icon_code =
                    normalize_icon_code(doc["daily"][i]["weather"][0]["icon"].as<String>());
                state.weather.daily[daily_out_idx].valid = true;
                Serial.printf("[Weather] Daily[%d] %s max=%.1f min=%.1f (fallback)\n",
                              daily_out_idx,
                              state.weather.daily[daily_out_idx].day_label.c_str(),
                              state.weather.daily[daily_out_idx].max_c,
                              state.weather.daily[daily_out_idx].min_c);
                daily_out_idx++;
            }

            state.weather.valid = true;
            state.weather.updated_ms = millis();

            Serial.printf("[Weather][DBG] Selected blocks: hourly=%d daily=%d\n", out_idx, daily_out_idx);
            Serial.printf("[Weather] Temp: %.1fC, icon=%s\n", state.weather.temp_c, state.weather.icon_code.c_str());
        } else {
            Serial.printf("[Weather] JSON parse error: %s\n", error.c_str());
        }
    } else {
        Serial.printf("[Weather] HTTP error: %d\n", http_code);
        if (http_code > 0) {
            String error_body = http.getString();
            int preview_len = error_body.length() > 240 ? 240 : error_body.length();
            String preview = error_body.substring(0, preview_len);
            preview.replace('\n', ' ');
            if (preview.length() > 0) {
                Serial.printf("[Weather][DBG] Error body: %s\n", preview.c_str());
            }
        }
        if (http_code == 401) {
            Serial.println("[Weather] API key might be invalid or inactive");
        }
    }

    http.end();
}
