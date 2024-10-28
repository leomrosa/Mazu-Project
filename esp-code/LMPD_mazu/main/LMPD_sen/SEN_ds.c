/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "SEN_ds.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "../LMPD_ch/MSG_handler.h"





esp_err_t ds18b20_trigger_temperature_conversion(onewire_bus_handle_t handle, const uint8_t *rom_number)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG_DS, "invalid 1-wire handle");

    ESP_RETURN_ON_ERROR(onewire_bus_reset(handle), TAG_DS, "error while resetting bus"); // reset bus and check if the device is present

    uint8_t tx_buffer[10];
    uint8_t tx_buffer_size;

    if (rom_number) { // specify rom id
        tx_buffer[0] = ONEWIRE_CMD_MATCH_ROM;
        tx_buffer[9] = DS18B20_CMD_CONVERT_TEMP;
        memcpy(&tx_buffer[1], rom_number, 8);
        tx_buffer_size = 10;
    } else { // skip rom id
        tx_buffer[0] = ONEWIRE_CMD_SKIP_ROM;
        tx_buffer[1] = DS18B20_CMD_CONVERT_TEMP;
        tx_buffer_size = 2;
    }

    ESP_RETURN_ON_ERROR(onewire_bus_write_bytes(handle, tx_buffer, tx_buffer_size),
                        TAG_DS, "error while triggering temperature convert");

    return ESP_OK;
}

esp_err_t ds18b20_get_temperature(onewire_bus_handle_t handle, const uint8_t *rom_number, float *temperature)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG_DS, "invalid 1-wire handle");
    ESP_RETURN_ON_FALSE(temperature, ESP_ERR_INVALID_ARG, TAG_DS, "invalid temperature pointer");

    ESP_RETURN_ON_ERROR(onewire_bus_reset(handle), TAG_DS, "error while resetting bus"); // reset bus and check if the device is present

    ds18b20_scratchpad_t scratchpad;

    uint8_t tx_buffer[10];
    uint8_t tx_buffer_size;

    if (rom_number) { // specify rom id
        tx_buffer[0] = ONEWIRE_CMD_MATCH_ROM;
        tx_buffer[9] = DS18B20_CMD_READ_SCRATCHPAD;
        memcpy(&tx_buffer[1], rom_number, 8);
        tx_buffer_size = 10;
    } else {
        tx_buffer[0] = ONEWIRE_CMD_SKIP_ROM;
        tx_buffer[1] = DS18B20_CMD_READ_SCRATCHPAD;
        tx_buffer_size = 2;
    }

    ESP_RETURN_ON_ERROR(onewire_bus_write_bytes(handle, tx_buffer, tx_buffer_size),
                        TAG_DS, "error while sending read scratchpad command");
    ESP_RETURN_ON_ERROR(onewire_bus_read_bytes(handle, (uint8_t *)&scratchpad, sizeof(scratchpad)),
                        TAG_DS, "error while reading scratchpad command");

    ESP_RETURN_ON_FALSE(onewire_check_crc8((uint8_t *)&scratchpad, 8) == scratchpad.crc_value, ESP_ERR_INVALID_CRC,
                        TAG_DS, "crc error");

    static const uint8_t lsb_mask[4] = { 0x07, 0x03, 0x01, 0x00 };
    uint8_t lsb_masked = scratchpad.temp_lsb & (~lsb_mask[scratchpad.configuration >> 5]); // mask bits not used in low resolution
    *temperature = (((int16_t)scratchpad.temp_msb << 8) | lsb_masked)  / 16.0f;

    return ESP_OK;
}

esp_err_t ds18b20_set_resolution(onewire_bus_handle_t handle, const uint8_t *rom_number, ds18b20_resolution_t resolution)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG_DS, "invalid 1-wire handle");

    ESP_RETURN_ON_ERROR(onewire_bus_reset(handle), TAG_DS, "error while resetting bus"); // reset bus and check if the device is present

    uint8_t tx_buffer[10];
    uint8_t tx_buffer_size;

    if (rom_number) { // specify rom id
        tx_buffer[0] = ONEWIRE_CMD_MATCH_ROM;
        tx_buffer[9] = DS18B20_CMD_WRITE_SCRATCHPAD;
        memcpy(&tx_buffer[1], rom_number, 8);
        tx_buffer_size = 10;
    } else {
        tx_buffer[0] = ONEWIRE_CMD_SKIP_ROM;
        tx_buffer[1] = DS18B20_CMD_WRITE_SCRATCHPAD;
        tx_buffer_size = 2;
    }

    ESP_RETURN_ON_ERROR(onewire_bus_write_bytes(handle, tx_buffer, tx_buffer_size),
                        TAG_DS, "error while sending read scratchpad command");

    tx_buffer[0] = 0;
    tx_buffer[1] = 0;
    tx_buffer[2] = resolution;
    ESP_RETURN_ON_ERROR(onewire_bus_write_bytes(handle, tx_buffer, 3),
                        TAG_DS, "error while sending write scratchpad command");

    return ESP_OK;
}

float ds18b20_readTemperature(onewire_bus_handle_t handle) {
    esp_err_t err;
    float temperature = 0.0;

    // set all sensors' temperature conversion resolution
    err = ds18b20_set_resolution(handle, NULL, DS18B20_RESOLUTION_12B);
    if (err != ESP_OK) {
        // Handle error
    }

    // trigger all sensors to start temperature conversion
    err = ds18b20_trigger_temperature_conversion(handle, NULL);
    if (err != ESP_OK) {
        // Handle error
    }

    
    // get temperature from sensors
    for (uint8_t i = 0; i < device_num; i++) {
        err = ds18b20_get_temperature(handle, device_rom_id[i], &temperature);
        if (err != ESP_OK) {
            // Handle error
        }

        ESP_LOGI(TAG_DS, "Temperature device %d: %.2f\n", i, temperature);
    }

    LastWaveParams.temperature = temperature;
    return temperature;
}



