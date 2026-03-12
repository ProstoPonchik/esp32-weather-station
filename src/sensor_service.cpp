#include "sensor_service.h"

#include <Arduino.h>
#include <Adafruit_SHT4x.h>
#include <Wire.h>

#include "app_config.h"

static Adafruit_SHT4x s_sht4;

void sensor_init(AppState &state)
{
    Serial.println("\n=== Initializing I2C Bus 0 (SHT41) ===");
    Serial.printf("Pins: SDA=%d, SCL=%d\n", APP_SHT41_SDA, APP_SHT41_SCL);
    Wire.begin(APP_SHT41_SDA, APP_SHT41_SCL);
    Wire.setTimeout(APP_I2C_TIMEOUT_MS);

    Serial.println("Initializing SHT41 sensor...");
    Serial.flush();
    if (!s_sht4.begin()) {
        Serial.println(" not found!");
        state.sht41_available = false;
    } else {
        Serial.println(" OK!");
        s_sht4.setPrecision(SHT4X_HIGH_PRECISION);
        s_sht4.setHeater(SHT4X_NO_HEATER);
        state.sht41_available = true;
    }
}

void sensor_read(const AppState &state, SensorSnapshot &snapshot)
{
    if (!state.sht41_available) {
        snapshot.valid = false;
        return;
    }

    sensors_event_t humidity;
    sensors_event_t temp;
    if (s_sht4.getEvent(&humidity, &temp)) {
        snapshot.temp_c = temp.temperature;
        snapshot.humidity_pct = humidity.relative_humidity;
        snapshot.valid = true;
    } else {
        snapshot.valid = false;
    }
}

