/**
 * @file biomech_calc.h
 * @brief Header file for biomechanics calculations and wave detection
 * @details This header defines functions and structures used to calculate biomechanics parameters, detect waves, and process cutback motion based on IMU and GPS data.
 *          Includes definitions for moving average windows and acceleration thresholds for wave detection.
 * @version 1.0
 * @date 2024-10-26
 * 
 * authors:
 * - Francisco Duarte
 * - Leonardo Rosa
 */

#ifndef BIOMECH_CALC_H
#define BIOMECH_CALC_H

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/i2c.h"
#include "ds3231.h"

static const char *TAG_BioMech = "BioMech";  // Logging tag for biomechanics module

// Definitions for moving average window sizes
#define MAX_WINDOW_SIZE_3 3
#define MAX_WINDOW_SIZE_4 4
#define MAX_WINDOW_SIZE_5 5
#define MAX_WINDOW_SIZE_6 6

// Threshold definitions for acceleration and wave detection
#define ACC_THRESHOLD 1.24 // Acceleration threshold for peak detection
#define FALL_THRESHOLD 1.0 // Acceleration threshold to detect fall
#define MIN_ACC_TIME_THRESHOLD 1000 // Minimum time to confirm wave initiation
#define ACC_END_THRESHOLD 1.15  // Threshold to detect end of wave
#define FALL_END_THRESHOLD 1.00 // Threshold to detect end of wave via fall
#define END_TIME_THRESHOLD 1000 // Time to confirm end of wave

#define SPEED_THRESHOLD 3.0             // Speed threshold to detect wave initiation
#define SPEED_END_THRESHOLD 2.0         // Speed threshold to detect wave end

#define ACC_DERIVATIVE_THRESHOLD_RISE 0.2  // Acceleration derivative threshold for rise detection
#define ACC_DERIVATIVE_THRESHOLD_FALL 0.2  // Acceleration derivative threshold for fall detection

#define CUTBACK_ANGLE_THRESHOLD 120.0 /*!< Threshold angle to detect cutback */
#define HEADING_STABILIZATION_THRESHOLD 10.0 /*!< Angle threshold to detect heading stabilization */
#define MINIMUM_DISTANCE 2.0 /*!< Minimum distance in meters to consider as a valid movement */

// Global variables and structures for wave detection and biomechanics calculations
extern bool in_wave;
extern int64_t wave_start_time;
extern int64_t wave_confirm_time;
extern int64_t wave_end_time;
extern int wave_count;
extern double total_wave_distance;

extern bool first_gps_reading;
extern int gps_reading_count;

extern i2c_dev_t rtc_dev;  // RTC device structure

// Function declarations

/**
 * @brief Calculates the moving average for a specified window size.
 * 
 * @param new_value The new value to add to the moving average.
 * @param window Array buffer holding the last values for averaging.
 * @param index Current index in the window array.
 * @param size Current size of the window.
 * @param window_size Fixed window size for averaging.
 * 
 * @return The calculated moving average.
 */
float calculate_moving_average(float new_value, float *window, int *index, int *size, int window_size);

/**
 * @brief Converts a struct tm time to milliseconds since epoch.
 * 
 * @param time Pointer to the struct tm time.
 * 
 * @return The time in milliseconds since epoch.
 */
int64_t convert_tm_to_ms(const struct tm *time);

/**
 * @brief Detects wave initiation, duration, and end based on IMU data.
 * 
 * @return esp_err_t result of the wave detection process.
 */
esp_err_t detect_waves_imu();

/**
 * @brief Detects wave initiation, duration, and end based on GPS data.
 * 
 * @return esp_err_t result of the wave detection process.
 */
esp_err_t detect_waves_gps();

/**
 * @brief Processes and detects a cutback motion.
 * 
 * @return esp_err_t result of the cutback detection process.
 */
esp_err_t process_cutback();

#endif // BIOMECH_CALC_H