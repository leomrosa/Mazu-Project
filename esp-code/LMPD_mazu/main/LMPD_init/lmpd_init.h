/**
 * @file imu_operations.h - LIMPID project
 * @brief .h file for handling IMU operations and device state toggling
 * @details This header defines functions and global variables for IMU operations, touch detection, and device toggling.
 * @version 1.0
 * @date 2024-10-24
 * 
 * @authors Francisco Duarte
 *          Leonardo Rosa
 **/

#ifndef IMU_OPERATIONS_H
#define IMU_OPERATIONS_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ds3231.h>
#include <time.h>

static const char *TAG_INIT= "INIT";

// Constant definitions
#define MAX_TOUCHES 3
#define TIME_WINDOW_SECONDS 3 // Time window for 3 touches (in seconds)

// Global variables
extern bool knock_trigger;
extern int touch_count;
extern time_t activation_time;
extern time_t deactivation_time;
extern time_t last_touch_time;

// Function declarations

/**
 * @brief Print duration in hours, minutes, and seconds.
 * 
 * @param[in] duration Duration in seconds.
 * @return esp_err_t
 */
esp_err_t print_duration(double duration);

/**
 * @brief Toggle device state.
 * 
 * @param[in] rtc_dev Pointer to the RTC I2C device structure.
 * @return esp_err_t
 */
esp_err_t toggle_device_state(i2c_dev_t *rtc_dev);

/**
 * @brief Handle touch detection.
 * 
 * @param[in] rtc_dev Pointer to the RTC I2C device structure.
 * @return esp_err_t
 */
esp_err_t touch_detected(i2c_dev_t *rtc_dev);

/**
 * @brief Main function to handle IMU operations.
 * 
 * @param[in] pvParameters Parameters for the FreeRTOS task.
 * @return esp_err_t
 */
esp_err_t handle_imu_operations(void *pvParameters);

/**
 * @brief Update tasks based on device state.
 * 
 * @param[in] device_on Current device state.
 */
void update_tasks_based_on_state(bool device_on);

#endif // IMU_OPERATIONS_H