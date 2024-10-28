/**
 * @file imu_operations.c - LIMPID project
 * @brief .c file to handle IMU operations and device state toggling
 * @details This driver handles IMU operations, touch detection, and toggling device state based on touch patterns and IMU data.
 * @version 1.0
 * @date 2024-10-24
 * 
 * @authors Francisco Duarte
 *          Leonardo Rosa
 **/

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ds3231.h>
#include <string.h>
#include <time.h>
#include "lmpd_init.h"
#include <math.h>
#include "LMPD_imu/IMU_mpu.h"
#include "LMPD_rtc/RTC_ds.h"

// Global variables
int touch_count = 0;
bool knock_trigger = false;
time_t last_touch_time = 0;
time_t activation_time = 0;
time_t deactivation_time = 0;
bool device_on = true; // Initial device state (on)

const float peak_threshold = 1.5;  // Upper limit for peak detection
const float sustained_threshold = 1.2; // Threshold for sustained movement detection
const int peak_duration_ms = 1000;  // Maximum duration for a peak (short peak)
const int sustained_duration_ms = 1000;  // Minimum duration for sustained movement

// Knock detection thresholds
const float knock_high_threshold = -0.93;  // Upper limit
const float knock_low_threshold = -2.0;  // Lower limit
const TickType_t debounce_delay = pdMS_TO_TICKS(200);  // Debounce delay to avoid false touches

// Detection thresholds for pitch and roll
#define PITCH_MIN -10.0   // Minimum pitch value
#define PITCH_MAX 10.0    // Maximum pitch value
#define ROLL_MIN -10.0    // Minimum roll value
#define ROLL_MAX 10.0     // Maximum roll value

float pitch = 0.0;
float roll = 0.0;

esp_err_t print_duration(double duration) {
    int hours = duration / 3600;
    int minutes = (int)(duration / 60) % 60;
    int seconds = (int)duration % 60;
    LastWaveParams.ss_hour =hours; 
    LastWaveParams.ss_min =minutes; 
    LastWaveParams.ss_sec =seconds;
    

    ESP_LOGI(TAG_INIT, "Device was on for %d hours, %d minutes, and %d seconds\n", hours, minutes, seconds);
    return ESP_OK;
}

esp_err_t toggle_device_state(i2c_dev_t *rtc_dev) {
    device_on = !device_on;
    update_tasks_based_on_state(device_on);

    struct tm current_time;
    if (ds3231_get_time(rtc_dev, &current_time) != ESP_OK) {
        ESP_LOGE(TAG_INIT, "Failed to get RTC time");
        return ESP_FAIL;
    }
    time_t now = mktime(&current_time);

    if (device_on) {
        activation_time = now;
        ESP_LOGI(TAG_INIT, "Device turned on");
        ESP_LOGI(TAG_INIT, "Power on date and time: %04d-%02d-%02d %02d:%02d:%02d\n",
                 current_time.tm_year + 1900, current_time.tm_mon + 1, current_time.tm_mday,
                 current_time.tm_hour, current_time.tm_min, current_time.tm_sec);
    } else {
        deactivation_time = now;
        double duration = difftime(deactivation_time, activation_time);
        print_duration(duration);
        ESP_LOGI(TAG_INIT, "Device turned off");
        ESP_LOGI(TAG_INIT, "Power off date and time: %04d-%02d-%02d %02d:%02d:%02d\n",
                 current_time.tm_year + 1900, current_time.tm_mon + 1, current_time.tm_mday,
                 current_time.tm_hour, current_time.tm_min, current_time.tm_sec);
    }
    return ESP_OK;
}

esp_err_t touch_detected(i2c_dev_t *rtc_dev) {
    struct tm current_time;
    if (ds3231_get_time(rtc_dev, &current_time) != ESP_OK) {
        ESP_LOGE(TAG_INIT, "Failed to get RTC time");
        return ESP_FAIL;
    }

    time_t now = mktime(&current_time);
    if (touch_count == 0) {
        last_touch_time = now;
        touch_count++;
        ESP_LOGI(TAG_INIT, "First touch detected!");
    } else {
        double time_diff = difftime(now, last_touch_time);
        if (time_diff <= TIME_WINDOW_SECONDS) {
            touch_count++;
            last_touch_time = now;
            ESP_LOGI(TAG_INIT, "Touch %d detected!", touch_count);
        } else {
            touch_count = 1;
            last_touch_time = now;
            ESP_LOGI(TAG_INIT, "Long time between touches, resetting count.");
        }
    }

    if (touch_count >= MAX_TOUCHES) {
        toggle_device_state(rtc_dev);  // Toggle device state after 3 touches
        touch_count = 0;  // Reset count
    }

    vTaskDelay(debounce_delay);  // Delay to prevent false touches
    return ESP_OK;
}

esp_err_t handle_imu_operations(void *pvParameters) {
    vector_t va, vg, vm;
    i2c_dev_t *rtc_dev = (i2c_dev_t *)pvParameters;
    static uint32_t peak_start_time = 0;

    ESP_ERROR_CHECK(get_accel_gyro_mag(&va, &vg, &vm));
    float accel_z = va.z;

    pitch = LastImuParams.pitch;
    roll = LastImuParams.roll;

    ESP_LOGI( TAG_INIT,"Pitch: %2.3f°, Roll: %2.3f°\n", pitch, roll);

    if (pitch >= PITCH_MIN && pitch <= PITCH_MAX && roll >= ROLL_MIN && roll <= ROLL_MAX) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (accel_z > knock_high_threshold || accel_z < knock_low_threshold) {
            if (peak_start_time == 0) peak_start_time = current_time;
            if (current_time - peak_start_time < peak_duration_ms) {
                printf("Touch peak detected on Z-axis!\n");
                touch_detected(rtc_dev);
            }
        } else {
            peak_start_time = 0;
        }
    } else {
        printf("Pitch or Roll out of range. Touch detection disabled.\n");
    }
    return ESP_OK;
}
