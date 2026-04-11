#include "components/sensors/drivers/bme680/bme680_driver.h"

#include <Arduino.h>
#include <Preferences.h>
#include <bsec2.h>
#include <math.h>

#include "core/app_config.h"

static Bsec2 s_bsec;
static Preferences s_bsec_prefs;

static bool s_available = false;
static uint8_t s_active_address = 0;
static uint32_t s_last_publish_ms = 0;
static uint32_t s_last_state_save_ms = 0;
static uint8_t s_prev_iaq_accuracy = 0;

static bool s_pending_ready = false;
static Bme680Snapshot s_pending = {};

static uint8_t s_bsec_state[BSEC_MAX_STATE_BLOB_SIZE] = {};

#if BME680_BSEC_CONFIG_28D
static const uint8_t s_bsec_config[] = {
    #include "../../../../../lib/Bosch-BSEC2-Library/src/config/bme680/bme680_iaq_33v_3s_28d/bsec_iaq.txt"
};
#else
static const uint8_t s_bsec_config[] = {
    #include "../../../../../lib/Bosch-BSEC2-Library/src/config/bme680/bme680_iaq_33v_3s_4d/bsec_iaq.txt"
};
#endif

static void print_bsec_status()
{
    if (s_bsec.status < BSEC_OK) {
        Serial.printf("[BSEC] error: %d\n", s_bsec.status);
    } else if (s_bsec.status > BSEC_OK) {
        Serial.printf("[BSEC] warning: %d\n", s_bsec.status);
    }

    if (s_bsec.sensor.status < BME68X_OK) {
        Serial.printf("[BME680] sensor error: %d\n", s_bsec.sensor.status);
    } else if (s_bsec.sensor.status > BME68X_OK) {
        Serial.printf("[BME680] sensor warning: %d\n", s_bsec.sensor.status);
    }
}

static bool load_bsec_state()
{
    size_t stored_len = s_bsec_prefs.getBytesLength(APP_BME680_BSEC_NVS_KEY);
    if (stored_len != BSEC_MAX_STATE_BLOB_SIZE) {
        Serial.println("[BSEC] no saved state in NVS");
        return true;
    }

    size_t read_len = s_bsec_prefs.getBytes(APP_BME680_BSEC_NVS_KEY, s_bsec_state, BSEC_MAX_STATE_BLOB_SIZE);
    if (read_len != BSEC_MAX_STATE_BLOB_SIZE) {
        Serial.printf("[BSEC] invalid saved state size: %u\n", static_cast<unsigned>(read_len));
        return false;
    }

    if (!s_bsec.setState(s_bsec_state)) {
        print_bsec_status();
        return false;
    }

    Serial.println("[BSEC] restored state from NVS");
    return true;
}

static bool save_bsec_state(uint32_t now_ms)
{
    if (!s_bsec.getState(s_bsec_state)) {
        print_bsec_status();
        return false;
    }

    size_t written = s_bsec_prefs.putBytes(APP_BME680_BSEC_NVS_KEY, s_bsec_state, BSEC_MAX_STATE_BLOB_SIZE);
    if (written != BSEC_MAX_STATE_BLOB_SIZE) {
        Serial.printf("[BSEC] failed to persist state, written=%u\n", static_cast<unsigned>(written));
        return false;
    }

    s_last_state_save_ms = now_ms;
    Serial.println("[BSEC] state saved to NVS");
    return true;
}

static void bsec_new_data_callback(const bme68xData data, const bsecOutputs outputs, const Bsec2 bsec)
{
    (void)data;
    (void)bsec;
    if (!outputs.nOutputs) return;

    Bme680Snapshot next = s_pending;

    bool has_temp = false;
    bool has_humi = false;
    bool has_pressure = false;
    bool has_gas = false;

    for (uint8_t i = 0; i < outputs.nOutputs; i++) {
        const bsecData output = outputs.output[i];
        switch (output.sensor_id) {
            case BSEC_OUTPUT_RAW_TEMPERATURE:
                next.temp_c = output.signal;
                has_temp = true;
                break;
            case BSEC_OUTPUT_RAW_HUMIDITY:
                next.humidity_pct = output.signal;
                has_humi = true;
                break;
            case BSEC_OUTPUT_RAW_PRESSURE:
                next.pressure_hpa = output.signal;
                has_pressure = true;
                break;
            case BSEC_OUTPUT_RAW_GAS:
                next.gas_resistance_ohm = output.signal;
                has_gas = true;
                break;
            case BSEC_OUTPUT_STATIC_IAQ:
                next.static_iaq = output.signal;
                next.iaq_accuracy = output.accuracy;
                next.iaq_valid = output.accuracy > 0;
                break;
            default:
                break;
        }
    }

    uint32_t sample_ms = static_cast<uint32_t>(outputs.output[0].time_stamp / INT64_C(1000000));
    next.updated_ms = sample_ms;
    next.last_bsec_run_ms = sample_ms;
    next.data_valid = has_temp && has_humi && has_pressure && has_gas;
    next.valid = next.data_valid;

    s_pending = next;
    s_pending_ready = true;
}

static bool bsec_try_begin(TwoWire &wire, uint8_t address)
{
    if (!s_bsec.begin(address, wire)) {
        print_bsec_status();
        return false;
    }

    if (APP_BME680_BSEC_ENABLED) {
        if (!s_bsec.setConfig(s_bsec_config)) {
            print_bsec_status();
            return false;
        }
    }

    // Output set for BME680 page + raw metrics
    bsecSensor sensor_list[] = {
        BSEC_OUTPUT_RAW_TEMPERATURE,
        BSEC_OUTPUT_RAW_HUMIDITY,
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_RAW_GAS,
        BSEC_OUTPUT_STATIC_IAQ,
        BSEC_OUTPUT_IAQ,
        BSEC_OUTPUT_STABILIZATION_STATUS,
        BSEC_OUTPUT_RUN_IN_STATUS,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    };

    if (!s_bsec.updateSubscription(sensor_list, ARRAY_LEN(sensor_list), BSEC_SAMPLE_RATE_LP)) {
        print_bsec_status();
        return false;
    }

    s_bsec.attachCallback(bsec_new_data_callback);

    if (!load_bsec_state()) {
        Serial.println("[BSEC] state load failed, running with fresh calibration");
    }

    s_active_address = address;
    Serial.printf("[BME680/BSEC] ready at 0x%02X\n", s_active_address);
    Serial.printf("[BSEC] version %u.%u.%u.%u\n",
                  s_bsec.version.major,
                  s_bsec.version.minor,
                  s_bsec.version.major_bugfix,
                  s_bsec.version.minor_bugfix);
    return true;
}

bool bme680_driver_init(TwoWire &wire)
{
    s_available = false;
    s_active_address = 0;
    s_pending = {};
    s_pending_ready = false;
    s_last_publish_ms = 0;
    s_last_state_save_ms = millis();
    s_prev_iaq_accuracy = 0;

    if (!APP_BME680_ENABLED) {
        Serial.println("[BME680/BSEC] disabled by config");
        return false;
    }

    if (!s_bsec_prefs.begin(APP_BME680_BSEC_NVS_NAMESPACE, false)) {
        Serial.println("[BSEC] failed to open NVS namespace");
        return false;
    }

    if (bsec_try_begin(wire, APP_BME680_ADDR_PRIMARY)) {
        s_available = true;
        return true;
    }

    if (bsec_try_begin(wire, APP_BME680_ADDR_SECONDARY)) {
        s_available = true;
        return true;
    }

    Serial.println("[BME680/BSEC] not found on configured addresses");
    return false;
}

void bme680_driver_run(uint32_t now_ms)
{
    if (!s_available) return;

    if (!s_bsec.run()) {
        print_bsec_status();
        return;
    }

    if (s_pending_ready) {
        bool time_to_save = (now_ms - s_last_state_save_ms) >= APP_BME680_BSEC_STATE_SAVE_INTERVAL_MS;
        bool reached_stable = (s_pending.iaq_accuracy >= 3) && (s_prev_iaq_accuracy < 3);
        if (time_to_save || reached_stable) {
            save_bsec_state(now_ms);
        }
        s_prev_iaq_accuracy = s_pending.iaq_accuracy;
    }
}

bool bme680_driver_read(Bme680Snapshot &snapshot, float reference_pressure_hpa, uint32_t now_ms)
{
    if (!s_available || !s_pending_ready) {
        return false;
    }

    if (now_ms - s_last_publish_ms < APP_BME680_PUBLISH_INTERVAL_MS) {
        return false;
    }

    snapshot = s_pending;

    float ref_hpa = reference_pressure_hpa;
    if (!(ref_hpa > 0.0f)) {
        ref_hpa = APP_BME680_SEA_LEVEL_HPA_DEFAULT;
    }

    if (snapshot.pressure_hpa > 0.0f && ref_hpa > 0.0f) {
        snapshot.altitude_m = 44330.0f * (1.0f - powf(snapshot.pressure_hpa / ref_hpa, 0.1903f));
    } else {
        snapshot.altitude_m = 0.0f;
    }

    snapshot.last_publish_ms = now_ms;
    snapshot.updated_ms = now_ms;
    snapshot.valid = snapshot.data_valid;

    s_last_publish_ms = now_ms;
    s_pending_ready = false;
    return true;
}

uint8_t bme680_driver_address()
{
    return s_active_address;
}
