#include "components/sensors/sensor_manager/sensor_manager.h"

#include <Arduino.h>
#include <Wire.h>

#include "components/sensors/drivers/bme680/bme680_driver.h"
#include "components/sensors/drivers/sht41/sht41_driver.h"
#include "core/app_config.h"

static uint32_t s_last_sht41_update_ms = 0;

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

static float bme680_reference_pressure_hpa(const AppState &state)
{
    if (state.weather.valid && state.weather.pressure_hpa >= 300.0f && state.weather.pressure_hpa <= 1200.0f) {
        return state.weather.pressure_hpa;
    }
    return APP_BME680_SEA_LEVEL_HPA_DEFAULT;
}

static void tick_bme680(AppState &state, uint32_t now_ms)
{
    if (!state.bme680_available) {
        state.sensors.bme680.valid = false;
        return;
    }

    bme680_driver_run(now_ms);

    Bme680Snapshot snapshot = state.sensors.bme680;
    if (bme680_driver_read(snapshot, bme680_reference_pressure_hpa(state), now_ms)) {
        state.sensors.bme680 = snapshot;
        promote_primary_from_bme680(state);
    }
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
        Serial.printf("[BME680/BSEC] active address: 0x%02X\n", bme680_driver_address());
    }

    s_last_sht41_update_ms = 0;
    state.sensors = {};
}

void sensor_manager_read_initial(AppState &state)
{
    read_sht41(state);
    tick_bme680(state, millis());

    if (!state.sensors.primary.valid) {
        state.sensors.primary.source = ClimateSource::None;
    }
}

void sensor_manager_tick(AppState &state)
{
    uint32_t now = millis();

    if (!state.sht41_available && !state.bme680_available) {
        state.sensors.primary.valid = false;
        state.sensors.primary.source = ClimateSource::None;
        return;
    }

    if (state.sht41_available && (now - s_last_sht41_update_ms >= APP_SENSOR_UPDATE_INTERVAL_MS)) {
        s_last_sht41_update_ms = now;
        read_sht41(state);
    }

    if (state.bme680_available) {
        tick_bme680(state, now);
    }

    if (!state.sensors.sht41.valid && !state.sensors.bme680.valid) {
        state.sensors.primary.valid = false;
        state.sensors.primary.source = ClimateSource::None;
    }
}

const SensorAggregate &sensor_manager_get(const AppState &state)
{
    return state.sensors;
}
