/* 
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_system.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "onewire_bus.h"
#include "driver/uart.h"
#include <math.h>
#include "driver/gpio.h"
#include "esp_sleep.h"

#include "driver/i2c.h"
#include "common.h"
#include "ahrs.h"
#include "mpu9250.h"
#include "calibrate.h"

#include "ds3231.h"
#include "i2cdev.h"

#include "LMPD_imu/IMU_mpu.c"
#include "LMPD_rtc/RTC_ds.c"
#include "LMPD_blt/BLT_spp.h"
#include "LMPD_sen/SEN_ds.h"
#include "LMPD_rom/ROM_msd.h"
#include "LMPD_gps/GPS_air.h"
#include "LMPD_biomech/Biomech_calc.h"
#include "LMPD_ch/MSG_handler.h"
#include "LMPD_init/lmpd_init.h"

#define GPIO_PIN_NUMBER GPIO_NUM_4 // Replace XX with the GPIO number you want to configure

const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
const bool esp_spp_enable_l2cap_ertm = true;
struct timeval time_new, time_old;
long data_num = 0;
const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;
bool bluetooth_connected = false;
bool surf_session_active = false;  // To control the state of surf_session_task

uint8_t blt = 0;

uint8_t device_num = 0;
uint8_t device_rom_id[EXAMPLE_ONEWIRE_MAX_DEVICES][8];
onewire_bus_handle_t handle_ds;

TaskHandle_t task_A0_handle = NULL;
TaskHandle_t task_A1_handle = NULL;
TaskHandle_t task_temperature_handle = NULL;

SemaphoreHandle_t actionT_semaphore;
SemaphoreHandle_t actionP_semaphore;
SemaphoreHandle_t actionA_semaphore;

SemaphoreHandle_t mutex_A0;
SemaphoreHandle_t mutex_A1;
SemaphoreHandle_t mutex_temperature;
SemaphoreHandle_t sd_write_mutex;

SemaphoreHandle_t configureSemaphore;
SemaphoreHandle_t writeSemaphore;

SemaphoreHandle_t bluetooth_semaphore; // Semaphore to notify offline_task about Bluetooth connection status
SemaphoreHandle_t xKnockSemaphore;

// Task handles
TaskHandle_t online_task_handle;
TaskHandle_t gps_task_handle;
TaskHandle_t imu_task_handle;
TaskHandle_t handle_imu_operations_task_handle;
TaskHandle_t tmp_task_handle;
TaskHandle_t biomech_task_gps_handle;
TaskHandle_t sd_task_imu_handle;
TaskHandle_t sd_task_handle;

SemaphoreHandle_t bluetooth_semaphore;
SemaphoreHandle_t xKnockSemaphore;
onewire_bus_handle_t handle_ds;

// Declaration of the start_bluetooth, stop_bluetooth, and online_task functions
void start_bluetooth();
void stop_bluetooth();
void online_task(void *pvParameters);  // Add this line

void update_tasks_based_on_state(bool device_on) { 
    if (device_on) {
        // Activates essential tasks and suspends recording and temperature tasks
        if (online_task_handle == NULL) {
            xTaskCreate(&online_task, "online_task", 4096, NULL, 5, &online_task_handle);
        }
        vTaskResume(gps_task_handle);
        vTaskResume(imu_task_handle);

        // Suspends recording tasks, ensuring the write operation is completed
        if (xSemaphoreTake(sd_write_mutex, portMAX_DELAY) == pdTRUE) {
            vTaskSuspend(sd_task_handle);
            vTaskSuspend(sd_task_imu_handle);
            xSemaphoreGive(sd_write_mutex);
        }
        
        vTaskSuspend(tmp_task_handle);
        vTaskSuspend(biomech_task_gps_handle);

    } else {
        // Deactivates online_task and activates recording and temperature tasks
        if (online_task_handle != NULL) {
            vTaskDelete(online_task_handle);
            online_task_handle = NULL;
        }
        
        stop_bluetooth();
        vTaskSuspend(gps_task_handle);
        vTaskSuspend(imu_task_handle);

        // Resumes recording and temperature tasks
        vTaskResume(tmp_task_handle);
        vTaskResume(biomech_task_gps_handle);
        
        // Waits for recording tasks to have exclusive access to the SD before resuming
        if (xSemaphoreTake(sd_write_mutex, portMAX_DELAY) == pdTRUE) {
            vTaskResume(sd_task_handle);
            vTaskResume(sd_task_imu_handle);
            xSemaphoreGive(sd_write_mutex);
        }
    }
}

void start_bluetooth() {
    esp_err_t ret;

    // Bluetooth controller configuration
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    // Initialize the Bluetooth controller if it is not ready
    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_IDLE) {
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(SPP_TAG, "Bluetooth controller init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK) {
        ESP_LOGE(SPP_TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(50));  // Delay for controller stabilization

    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(SPP_TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(SPP_TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return;
    }

    // Register the GAP and SPP callbacks
    ret = esp_bt_gap_register_callback(esp_bt_gap_cb);
    if (ret != ESP_OK) {
        ESP_LOGE(SPP_TAG, "GAP register failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_spp_register_callback(esp_spp_cb);
    if (ret != ESP_OK) {
        ESP_LOGE(SPP_TAG, "SPP register failed: %s", esp_err_to_name(ret));
        return;
    }

    // SPP configuration
    esp_spp_cfg_t bt_spp_cfg = {
        .mode = ESP_SPP_MODE_CB,
        .enable_l2cap_ertm = true,
        .tx_buffer_size = 0,
    };

    ret = esp_spp_enhanced_init(&bt_spp_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(SPP_TAG, "SPP init failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(SPP_TAG, "Bluetooth started successfully");
}

void stop_bluetooth() {
    esp_spp_deinit();
    vTaskDelay(pdMS_TO_TICKS(50));

    esp_bluedroid_disable();
    while (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    esp_bluedroid_deinit();

    esp_bt_controller_disable();
    while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    esp_bt_controller_deinit();
    
    ESP_LOGI(SPP_TAG, "Bluetooth stopped successfully");
}

void online_task(void *pvParameters) { 
    // Variável estática para contar quantas vezes a task foi iniciada
    static int start_count = 0;

    // Incrementa o contador de inícios da task
    start_count++;

    // Start Bluetooth quando a task começa
    start_bluetooth();
    vTaskDelay(pdMS_TO_TICKS(100));

    // Chama a função na segunda vez que a task é executada
    if (start_count >= 2) {
        esp_err_t result = LMPD_device_copy_last_valid_line();
        if (result == ESP_OK) {
            ESP_LOGI("ONLINE_TASK", "Última linha válida copiada com sucesso.");
        } else {
            ESP_LOGE("ONLINE_TASK", "Erro ao copiar a última linha válida.");
        }
    }

    char bda_str[18] = {0};
    ESP_LOGI(SPP_TAG, "Own address:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));

    while (1) {
        if (!bluetooth_connected) {
            xSemaphoreGive(bluetooth_semaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Stop Bluetooth quando a task é suspensa
    stop_bluetooth();
    vTaskDelay(pdMS_TO_TICKS(100));
}
void sd_task_imu(void *pvParameters)
{
    while (1)
    {
        if (xSemaphoreTake(sd_write_mutex, portMAX_DELAY) == pdTRUE) {
            // Perform write operation on SD card
            save_waveimu_params_to_csv(&LastWaveIMUParams);
            save_wave_params_to_csv(&LastWaveParams);
            xSemaphoreGive(sd_write_mutex);  // Release the mutex after writing
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void sd_task(void *pvParameters)
{
    while (1)
    {
        if (xSemaphoreTake(sd_write_mutex, portMAX_DELAY) == pdTRUE) {
            // Perform write operations on SD card
            save_wavegps_params_to_csv(&LastWaveGPSParams);
            save_tmp_params_to_csv();
            xSemaphoreGive(sd_write_mutex);  // Release the mutex after writing
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void biomech_task_gps(void *pvParameters)
{
    while (1)
        {
        detect_waves_gps();
        }
}

void tmp_task(void *pvParameters)
{
    while (1)
    {
        ds18b20_readTemperature(handle_ds);
        vTaskDelay(1000/ portTICK_PERIOD_MS);
    }
}

void handle_imu_operations_task(void *pvParameters) {
    i2c_dev_t *rtc_dev = (i2c_dev_t *)pvParameters;

    while (1) {
        handle_imu_operations(rtc_dev);
        vTaskDelay(pdMS_TO_TICKS(100));  // Add a small delay to avoid CPU overload
    }
}

void imu_task(void *pvParameters)
{
    while (1)
    {
        run_imu(NULL);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void gps_task(void *pvParameters)
{
        air530z_task(NULL);
}

void app_main(void)
{
    // Initialize NVS before any Bluetooth or WiFi initialization
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // If there is not enough space or a new version, erase and reinitialize the NVS
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Release BLE memory once
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    // Initialize mutex for SD card write operations
    sd_write_mutex = xSemaphoreCreateMutex();
    if (sd_write_mutex == NULL) {
        ESP_LOGE("SD", "Error creating mutex for SD write");
        return;
    }

    bluetooth_semaphore = xSemaphoreCreateBinary();
    xKnockSemaphore = xSemaphoreCreateBinary();

    if (bluetooth_semaphore == NULL)
    {
        // Semaphore creation failed
        // Handle error
    }

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;            // Disable interrupt
    io_conf.mode = GPIO_MODE_INPUT;                   // Set as input mode
    io_conf.pin_bit_mask = (1ULL << GPIO_PIN_NUMBER); // Bit mask of the pins that you want to set
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;          // Enable pull-up resistor
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;     // Disable pull-down resistor
    gpio_config(&io_conf);
    // ------------------------------------------ DS18B20 INIT SECTION ----------------------------------------//

    onewire_rmt_config_t config = {
        .gpio_pin = EXAMPLE_ONEWIRE_GPIO_PIN,
        .max_rx_bytes = 10
    };

    ESP_ERROR_CHECK(onewire_new_bus_rmt(&config, &handle_ds));
    ESP_LOGI(TAG_DS, "1-wire bus installed");

    onewire_rom_search_context_handler_t context_handler;
    ESP_ERROR_CHECK(onewire_rom_search_context_create(handle_ds, &context_handler));

    do {
        esp_err_t search_result = onewire_rom_search(context_handler);

        if (search_result == ESP_ERR_INVALID_CRC) {
            continue;
        } else if (search_result == ESP_FAIL || search_result == ESP_ERR_NOT_FOUND) {
            break;
        }

        ESP_ERROR_CHECK(onewire_rom_get_number(context_handler, device_rom_id[device_num]));
        ESP_LOGI(TAG_DS, "found device with rom id " ONEWIRE_ROM_ID_STR, ONEWIRE_ROM_ID(device_rom_id[device_num]));
        device_num++;
    } while (device_num < EXAMPLE_ONEWIRE_MAX_DEVICES);

    ESP_ERROR_CHECK(onewire_rom_search_context_delete(context_handler));
    ESP_LOGI(TAG_DS, "%d device%s found on 1-wire bus", device_num, device_num > 1 ? "s" : "");

    // ------------------------------------------ I2C INIT SECTION ----------------------------------------//

    // ESP_ERROR_CHECK(LMPD_I2C_init());
    // ESP_LOGI(TAG_DS, "I2C initialized successfully");

    ESP_ERROR_CHECK(i2c_mpu9250_init(&cal));

    ahrs_init(SAMPLE_FREQ_Hz, 0.8);

    ESP_ERROR_CHECK(i2cdev_init());

    // ------------------------------------------ SPI INIT SECTION ----------------------------------------//

    ESP_ERROR_CHECK(sd_card_init());

    LMPD_SYSTEM_save_parameters();

    // ------------------------------------------ UART INIT SECTION ----------------------------------------//

    // Initialize the GPS once
    gps_initialize();

    // ------------------------------------------ TASKS CREATE SECTION ----------------------------------------//
    xTaskCreate(&online_task, "online_task", 8192, NULL, 5, &online_task_handle);
    xTaskCreate(&gps_task, "gps_task", 8192, NULL, 4, &gps_task_handle);
    xTaskCreate(&imu_task, "imu_task", 4096, NULL, 3, &imu_task_handle);
    xTaskCreate(&handle_imu_operations_task, "imu_operations_task", 4096, NULL, 5, &handle_imu_operations_task_handle);
    xTaskCreate(&tmp_task, "tmp_task", 4096, NULL, 3, &tmp_task_handle);
    xTaskCreate(&biomech_task_gps, "biomech_task_gps", 4096, NULL, 4, &biomech_task_gps_handle);
    xTaskCreate(&sd_task_imu, "sd_task_imu", 4096, NULL, 2, &sd_task_imu_handle);
    xTaskCreate(&sd_task, "sd_task", 4096, NULL, 2, &sd_task_handle);

    update_tasks_based_on_state(true);
}
