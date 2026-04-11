#include "components/sensors/drivers/sht41/sht41_driver.h"

#include <Adafruit_SHT4x.h>
#include <Arduino.h>

static Adafruit_SHT4x s_sht4;
static bool s_available = false;

bool sht41_driver_init(TwoWire &wire)
{
    Serial.println("Initializing SHT41 sensor...");
    if (!s_sht4.begin(&wire)) {
        Serial.println("[SHT41] not found");
        s_available = false;
        return false;
    }

    s_sht4.setPrecision(SHT4X_HIGH_PRECISION);
    s_sht4.setHeater(SHT4X_NO_HEATER);
    s_available = true;
    Serial.println("[SHT41] ready");
    return true;
}

bool sht41_driver_read(Sht41Snapshot &snapshot)
{
    if (!s_available) {
        snapshot.valid = false;
        return false;
    }

    sensors_event_t humidity;
    sensors_event_t temp;
    if (!s_sht4.getEvent(&humidity, &temp)) {
        snapshot.valid = false;
        return false;
    }

    snapshot.temp_c = temp.temperature;
    snapshot.humidity_pct = humidity.relative_humidity;
    snapshot.valid = true;
    snapshot.updated_ms = millis();
    return true;
}
