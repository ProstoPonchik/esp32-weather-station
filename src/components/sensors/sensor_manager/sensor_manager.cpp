#include "components/sensors/sensor_manager/sensor_manager.h"

#include <Arduino.h>
#include <Wire.h>

#include "components/sensors/drivers/bme680/bme680_driver.h"
#include "components/sensors/drivers/sht41/sht41_driver.h"
#include "core/app_config.h"

static bool s_next_read_sht41 = true;

static void promote_primary_from_sht41(AppState &state)
{
    if (!state.sensors.sht41.valid) return;

    state.sensors.primary.temp_c = state.sensors.sht41.temp_c;
    state.sensors.primary.humidity_pct = state.sensors.sht41.humidity_pct;
    state.sensors.primary.source = ClimateSource::Sht41;
    state.sensors.primary.valid = true;
    state.sensors.primary.updated_ms = state.sensors.sht41.updated_ms;
}

static void promote_primary_from_bme680(AppState &state)
{
    if (!state.sensors.bme680.valid) return;

    state.sensors.primary.temp_c = state.sensors.bme680.temp_c;
    state.sensors.primary.humidity_pct = state.sensors.bme680.humidity_pct;
    state.sensors.primary.source = ClimateSource::Bme680;
    state.sensors.primary.valid = true;
    state.sensors.primary.updated_ms = state.sensors.bme680.updated_ms;
}

static void read_sht41(AppState &state)
{
    if (!state.sht41_available) {
        state.sensors.sht41.valid = false;
        return;
    }

    Sht41Snapshot snapshot = state.sensors.sht41;
    if (sht41_driver_read(snapshot)) {
        state.sensors.sht41 = snapshot;
        promote_primary_from_sht41(state);
        return;
    }

    state.sensors.sht41.valid = false;
}

static void read_bme680(AppState &state)
{
    if (!state.bme680_available) {
        state.sensors.bme680.valid = false;
        return;
    }

    Bme680Snapshot snapshot = state.sensors.bme680;
    if (bme680_driver_read(snapshot)) {
        state.sensors.bme680 = snapshot;
        promote_primary_from_bme680(state);
        return;
    }

    state.sensors.bme680.valid = false;
}

void sensor_manager_init(AppState &state)
{
    Serial.println("\n=== Initializing I2C Bus 0 (Sensors) ===");
    Serial.printf("Pins: SDA=%d, SCL=%d\n", APP_SHT41_SDA, APP_SHT41_SCL);
    Wire.begin(APP_SHT41_SDA, APP_SHT41_SCL);
    Wire.setTimeout(APP_I2C_TIMEOUT_MS);

    state.sht41_available = sht41_driver_init(Wire);
    state.bme680_available = bme680_driver_init(Wire);

    if (state.bme680_available) {
        Serial.printf("[BME680] active address: 0x%02X\n", bme680_driver_address());
    }

    s_next_read_sht41 = true;
    state.sensors = {};
}

void sensor_manager_read_initial(AppState &state)
{
    read_sht41(state);
    read_bme680(state);

    if (!state.sensors.primary.valid) {
        state.sensors.primary.source = ClimateSource::None;
    }
}

void sensor_manager_tick(AppState &state)
{
    if (!state.sht41_available && !state.bme680_available) {
        state.sensors.primary.valid = false;
        state.sensors.primary.source = ClimateSource::None;
        return;
    }

    if (state.sht41_available && state.bme680_available) {
        if (s_next_read_sht41) {
            read_sht41(state);
        } else {
            read_bme680(state);
        }
        s_next_read_sht41 = !s_next_read_sht41;
        return;
    }

    if (state.sht41_available) {
        read_sht41(state);
        return;
    }

    read_bme680(state);
}

const SensorAggregate &sensor_manager_get(const AppState &state)
{
    return state.sensors;
}
