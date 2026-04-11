#include "components/sensors/drivers/bme680/bme680_driver.h"

#include <Arduino.h>
#include <bme68xLibrary.h>

#include "core/app_config.h"

static Bme68x s_bme680;
static bool s_available = false;
static uint8_t s_active_address = 0;

static bool bme680_try_begin(TwoWire &wire, uint8_t address)
{
    s_bme680.begin(address, wire);
    int8_t status = s_bme680.checkStatus();
    if (status == BME68X_ERROR) {
        Serial.printf("[BME680] begin failed at 0x%02X: %s\n", address, s_bme680.statusString().c_str());
        return false;
    }

    if (status == BME68X_WARNING) {
        Serial.printf("[BME680] begin warning at 0x%02X: %s\n", address, s_bme680.statusString().c_str());
    }

    s_bme680.setTPH();
    s_bme680.setHeaterProf(APP_BME680_HEATER_TEMP_C, APP_BME680_HEATER_DURATION_MS);

    status = s_bme680.checkStatus();
    if (status == BME68X_ERROR) {
        Serial.printf("[BME680] config failed at 0x%02X: %s\n", address, s_bme680.statusString().c_str());
        return false;
    }

    if (status == BME68X_WARNING) {
        Serial.printf("[BME680] config warning at 0x%02X: %s\n", address, s_bme680.statusString().c_str());
    }

    s_active_address = address;
    Serial.printf("[BME680] ready at 0x%02X\n", s_active_address);
    return true;
}

bool bme680_driver_init(TwoWire &wire)
{
    s_available = false;
    s_active_address = 0;

    if (!APP_BME680_ENABLED) {
        Serial.println("[BME680] disabled by config");
        return false;
    }

    if (bme680_try_begin(wire, APP_BME680_ADDR_PRIMARY)) {
        s_available = true;
        return true;
    }

    if (bme680_try_begin(wire, APP_BME680_ADDR_SECONDARY)) {
        s_available = true;
        return true;
    }

    Serial.println("[BME680] not found on configured addresses");
    return false;
}

bool bme680_driver_read(Bme680Snapshot &snapshot)
{
    if (!s_available) {
        snapshot.valid = false;
        return false;
    }

    s_bme680.setOpMode(BME68X_FORCED_MODE);
    uint32_t meas_us = s_bme680.getMeasDur(BME68X_FORCED_MODE);
    uint32_t wait_ms = (meas_us / 1000U) + APP_BME680_MEAS_EXTRA_DELAY_MS;
    delay(wait_ms);

    if (s_bme680.fetchData() == 0) {
        snapshot.valid = false;
        return false;
    }

    bme68xData data = {};
    s_bme680.getData(data);

    if (s_bme680.checkStatus() == BME68X_ERROR) {
        snapshot.valid = false;
        return false;
    }

    snapshot.temp_c = data.temperature;
    snapshot.humidity_pct = data.humidity;
    snapshot.pressure_hpa = data.pressure / 100.0f;
    snapshot.gas_resistance_ohm = data.gas_resistance;
    snapshot.valid = true;
    snapshot.updated_ms = millis();
    return true;
}

uint8_t bme680_driver_address()
{
    return s_active_address;
}
