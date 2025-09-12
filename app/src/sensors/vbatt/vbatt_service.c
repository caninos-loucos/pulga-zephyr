/*
 * Copyright (c) 2018-2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <integration/timestamp/timestamp_service.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <sensors/vbatt/vbatt_service.h>

LOG_MODULE_REGISTER(vbatt_service, CONFIG_APP_LOG_LEVEL);

#define VBATT DT_PATH(vbatt)
#define ADC DT_IO_CHANNELS_CTLR(VBATT)
#define CHANNEL DT_CHILD(ADC, channel_1)

/**
 * DEFINITIONS
 */

static SensorAPI vbatt_api = {0};

struct divider_config
{
    uint8_t io_channel;
    struct gpio_dt_spec power_gpios;
    uint32_t output_ohm;
    uint32_t full_ohm;
};

static const struct divider_config divider_config = {
    .io_channel = DT_IO_CHANNELS_INPUT(VBATT),
    .power_gpios = GPIO_DT_SPEC_GET_OR(VBATT, power_gpios, {}),
    .output_ohm = DT_PROP(VBATT, output_ohms),
    .full_ohm = DT_PROP(VBATT, full_ohms),
};

struct divider_data
{
    const struct device *adc;
    const struct adc_channel_cfg ch_cfg;
    struct adc_sequence adc_seq;
    int16_t raw;
};

static struct divider_data divider_data = {
    .adc = DEVICE_DT_GET(ADC),
    .ch_cfg = ADC_CHANNEL_CFG_DT(CHANNEL),
};

int divider_setup(void);

int sample_battery(int32_t *millivolts);

void warn_low_battery(struct k_work *work)
{
    LOG_WRN("Battery < %d.%03d V!",
            CONFIG_LOW_BATT_THRESH / 1000,
            CONFIG_LOW_BATT_THRESH % 1000);
}
K_WORK_DELAYABLE_DEFINE(low_battery_work, warn_low_battery);

/**
 * IMPLEMENTATIONS
 */

static int divider_setup(void)
{
    const struct gpio_dt_spec *power_gpios = &divider_config.power_gpios;

    int error;

    if (!device_is_ready(divider_data.adc))
    {
        LOG_ERR("ADC device is not ready %s", divider_data.adc->name);
        return -ENOENT;
    }

    if (power_gpios->port)
    {
        if (!device_is_ready(power_gpios->port))
        {
            LOG_ERR("%s: device not ready", power_gpios->port->name);
            return -ENOENT;
        }

        error = gpio_pin_configure_dt(power_gpios, GPIO_OUTPUT_INACTIVE);
        if (error)
        {
            LOG_ERR("Failed to control feed %s.%u: %d",
                    power_gpios->port->name, power_gpios->pin, error);
            return error;
        }
    }

    divider_data.adc_seq = (struct adc_sequence){
        .channels = BIT(divider_data.ch_cfg.channel_id),
        .buffer = &divider_data.raw,
        .buffer_size = sizeof(divider_data.raw),
        .oversampling = DT_PROP(CHANNEL, zephyr_oversampling),
        .resolution = DT_PROP(CHANNEL, zephyr_resolution),
        .calibrate = true,
    };

    error = adc_channel_setup(divider_data.adc, &divider_data.ch_cfg);

    return error;
}

// Gets and initializes device
static int init_sensor()
{
    LOG_DBG("Initializing vbatt");

    int error = divider_setup();
    if (error != 0)
    {
        LOG_ERR("Error setting up divider on AIN%u got %d",
                divider_config.io_channel, error);
        return error;
    }

    int raw = divider_data.raw;
    error = sample_battery(&raw);
    divider_data.adc_seq.calibrate = false;

    LOG_INF("Battery set up");
    return 0;
}

int sample_battery(int32_t *millivolts)
{
    int error = adc_read(divider_data.adc, &divider_data.adc_seq);
    if (error)
    {
        LOG_ERR("Error reading adc sequence: %d", error);
        return error;
    }

    int32_t raw = divider_data.raw;
    error = adc_raw_to_millivolts(
        adc_ref_internal(divider_data.adc),
        divider_data.ch_cfg.gain,
        divider_data.adc_seq.resolution,
        &raw);
    if (error)
    {
        LOG_ERR("Error converting buffer data: %d", error);
        return error;
    }

    *millivolts = raw * divider_config.full_ohm / divider_config.output_ohm;
    
    LOG_DBG("raw: %u mV, scaled: %d mV", raw, (*millivolts));
    return 0;
}

// Reads sensor measurements and stores them in buffer
static void read_sensor_values()
{
    LOG_DBG("Reading vbatt");

    SensorModelVbatt vbatt_model;
    uint32_t vbatt_data[MAX_32_WORDS];

    int32_t millivolts = divider_data.raw;
    int error = sample_battery(&millivolts);

    while (error == -EAGAIN)
    {
        LOG_WRN("fetch sample from \"%s\" failed: %d, trying again", divider_data.adc->name, error);
        error = sample_battery(&millivolts);
    }

    if (error)
    {
        LOG_ERR("fetch sample from \"%s\" failed: %d", divider_data.adc->name, error);
        return;
    }

    sensor_value_from_milli(&vbatt_model.voltage, millivolts);
#ifndef CONFIG_EVENT_TIMESTAMP_NONE
    vbatt_model.timestamp = get_current_timestamp();
#endif /* CONFIG_EVENT_TIMESTAMP_NONE */

    memcpy(&vbatt_data, &vbatt_model, sizeof(SensorModelVbatt));
    if (insert_in_buffer(&app_buffer, vbatt_data, VBATT_MODEL, 0, VBATT_MODEL_WORDS) != 0)
    {
        LOG_ERR("Failed to insert data in ring buffer.");
    }

    if (millivolts < CONFIG_LOW_BATT_THRESH)
    {
        k_work_schedule(&low_battery_work, K_MSEC(2000));
    }
}

// Register vbatt sensor callbacks
SensorAPI *register_vbatt_callbacks()
{
    LOG_DBG("Registering vbatt callbacks");
    vbatt_api.init_sensor = init_sensor;
    vbatt_api.read_sensor_values = read_sensor_values;
    vbatt_api.data_model_api = register_vbatt_model_callbacks();
    return &vbatt_api;
}