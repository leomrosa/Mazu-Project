/**
 * @file imu.h - LIMPID project
 * @brief Header file for IMU (Inertial Measurement Unit) functions
 * @details Contains declarations for IMU initialization and functions to handle IMU data transformations and tasks.
 * @version 1.0
 * @date 2024-04-13
 * 
 * authors Francisco Duarte
 *         Leonardo Rosa
 **/

#ifndef IMU_H_
#define IMU_H_

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
#include "ahrs.h"
#include "mpu9250.h"
#include "calibrate.h"
#include "common.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_FREQ_Hz (CONFIG_SAMPLE_RATE_Hz)
#define SAMPLE_INTERVAL_MS (200 / SAMPLE_FREQ_Hz) // Sample Rate in milliseconds
#define DEG2RAD(deg) (deg * M_PI / 180.0f)

#define I2C_MASTER_NUM I2C_NUM_0 /*!< I2C port number for master dev */

static const char *TAG_IMU = "Imu";

/**
 * @brief Transform accelerometer and gyroscope readings to device orientation.
 * 
 * @param[in,out] v Pointer to the vector to be transformed.
 * 
 * @return esp_err_t
 */
esp_err_t transform_accel_gyro(vector_t *v);

/**
 * @brief Transform magnetometer readings to device orientation.
 * 
 * @param[in,out] v Pointer to the vector to be transformed.
 * 
 * @return esp_err_t
 */
esp_err_t transform_mag(vector_t *v);

/**
 * @brief Update global IMU orientation data.
 * 
 * @param[in] heading Heading angle.
 * @param[in] pitch Pitch angle.
 * @param[in] roll Roll angle.
 * 
 * @return esp_err_t
 */
esp_err_t update_imu_orientation(float heading, float pitch, float roll);

/**
 * @brief Pause the task based on sample interval.
 * 
 * @return esp_err_t
 */
esp_err_t pause(void);

/**
 * @brief Run the IMU task to read sensor data and perform AHRS algorithm.
 * 
 * @param[in] pvParameters Parameters for the FreeRTOS task.
 * 
 * @return esp_err_t
 */
esp_err_t run_imu(void *pvParameters);

#endif /* IMU_H_ */
