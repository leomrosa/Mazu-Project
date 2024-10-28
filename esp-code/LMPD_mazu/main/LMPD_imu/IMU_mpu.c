/**
 * @file imu_operations.c - LIMPID project
 * @brief .c file for IMU operations and transformations
 * @details Implements transformations, IMU data handling, and running the main IMU task.
 * @version 1.0
 * @date 2024-04-13
 * 
 * @authors Francisco Duarte
 *          Leonardo Rosa
 **/

#include "IMU_mpu.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_task_wdt.h"
#include "driver/i2c.h"
#include "common.h"
#include "ahrs.h"
#include "mpu9250.h"
#include "calibrate.h"
#include "../LMPD_ch/MSG_handler.h"
#include "../LMPD_rtc/RTC_ds.h"

#define I2C_MASTER_NUM I2C_NUM_0 /*!< I2C port number for master dev */

calibration_t cal = {
    .gyro_bias_offset = {.x = 6.769912, .y = 0.841067, .z = 0.084878},
    .accel_offset = {.x = -0.009498, .y = 0.052851, .z = -0.076125},
    .accel_scale_lo = {.x = 0.995050, .y = 1.019846, .z = 0.960147},
    .accel_scale_hi = {.x = -1.006273, .y = -0.981114, .z = -1.059381},
    .mag_offset = {.x = 75.804688, .y = 47.072266, .z = -48.367188},
    .mag_scale = {.x = 0.928344, .y = 1.087757, .z = 0.996521}
};

esp_err_t transform_accel_gyro(vector_t *v) {
    float x = v->x;
    float y = v->y;
    float z = v->z;

    v->x = y;    // Swap X and Y axes
    v->y = -x;   // Invert X axis to correct the sign
    v->z = -z;   // Invert Z axis to correct roll sign
    return ESP_OK;
}

esp_err_t transform_mag(vector_t *v) {
    float x = v->x;
    float y = v->y;
    float z = v->z;

    v->x = y;    // Swap X and Y axes
    v->y = -x;   // Invert X axis to correct the sign
    v->z = -z;   // Invert Z axis to correct roll sign
    return ESP_OK;
}

esp_err_t update_imu_orientation(float heading, float pitch, float roll) {
    LastImuParams.heading = heading;
    LastImuParams.pitch = pitch;
    LastImuParams.roll = roll;
    return ESP_OK;
}

esp_err_t pause(void) {
    static uint64_t start = 0;
    uint64_t end = xTaskGetTickCount() * 100 / configTICK_RATE_HZ;

    if (start == 0) {
        start = xTaskGetTickCount() / configTICK_RATE_HZ;
    }

    int32_t elapsed = end - start;
    if (elapsed < SAMPLE_INTERVAL_MS) {
        vTaskDelay((SAMPLE_INTERVAL_MS - elapsed) / portTICK_PERIOD_MS);
    }
    start = xTaskGetTickCount() * 100 / configTICK_RATE_HZ;
    return ESP_OK;
}

esp_err_t run_imu(void *pvParameters) {
    uint64_t i = 0;
    vector_t va, vg, vm;

    ESP_ERROR_CHECK(get_accel_gyro_mag(&va, &vg, &vm));
    ESP_ERROR_CHECK(transform_accel_gyro(&va));
    ESP_ERROR_CHECK(transform_accel_gyro(&vg));
    ESP_ERROR_CHECK(transform_mag(&vm));

    ahrs_update(DEG2RAD(vg.x), DEG2RAD(vg.y), DEG2RAD(vg.z), va.x, va.y, va.z, vm.x, vm.y, vm.z);

    if (i++ % 10 == 0) {
        float heading, pitch, roll;
        ahrs_get_euler_in_degrees(&heading, &pitch, &roll);
        ESP_ERROR_CHECK(update_imu_orientation(heading, pitch, roll));
        ESP_LOGI(TAG_IMU, "heading: %2.3f°, pitch: %2.3f°, roll: %2.3f°\n", heading, pitch, roll);
        ESP_ERROR_CHECK(pause());
    }
    return ESP_OK;
}
