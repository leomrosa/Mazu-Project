/** 
 * @file MSG_handler.h - LIMPID project
 * @brief Header file for system operations management
 * @details This header defines functions and structures for handling system responses over Bluetooth SPP, as well as managing sensor data and system power modes.
 * @version 1.0
 * @date 2024-03-24
 * 
 * authors Francisco Duarte
 *         Leonardo Rosa
 **/

#ifndef MSG_HANDLER_H_
#define MSG_HANDLER_H_

#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_bt.h"
#include "esp_system.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "onewire_bus.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/semphr.h"
#include <time.h>

#include "../LMPD_sen/SEN_ds.h"
#include "../LMPD_rom/ROM_msd.h"

#define GPIO_PIN_POWER  GPIO_NUM_5   // GPIO pin to control power for breakout boards
#define GPIO_PIN_STAT   GPIO_NUM_13  // GPIO pin to check the battery status

#define POWER_MODE_ON 1              // Power mode ON flag
#define POWER_MODE_OFF 0             // Power mode OFF flag

#define MAX_LINE_LENGTH 500          // Max length for buffer lines in reading CSV data

static const char *BAT_TAG = "CHARGE"; // Tag for logging battery status
static const char *TAG_MSG = "MSG";    // Tag for logging messages

// Global semaphores for task synchronization
extern SemaphoreHandle_t actionT_semaphore;
extern SemaphoreHandle_t actionP_semaphore;
extern SemaphoreHandle_t actionA_semaphore;

/* 
 * @struct WaterParams
 * @brief Structure to store the last temperature reading from the sensor
 */
typedef struct {
    float Temperature;  // Stores the last temperature value read from the sensor
} WaterParams;

/* 
 * @struct DateTime
 * @brief Structure to store the current date and time
 */
typedef struct {
    int year;   // Year component of date
    int month;  // Month component of date
    int day;    // Day component of date
    int hour;   // Hour component of time
    int min;    // Minute component of time
    int sec;    // Second component of time
} DateTime;

/* 
 * @struct ImuParams
 * @brief Structure to store the last IMU sensor readings and metadata
 */
typedef struct {
    float heading;  // Last heading reading
    float pitch;    // Last pitch reading
    float roll;     // Last roll reading
    int year;       // Year when reading was taken
    int month;      // Month when reading was taken
    int day;        // Day when reading was taken
    int hour;       // Hour when reading was taken
    int min;        // Minute when reading was taken
    int sec;        // Second when reading was taken
    int cutback;    // Last cutback value
    int wave;       // Last wave data point
} ImuParams;

/* 
 * @struct GpsParams
 * @brief Structure to store the last GPS data readings and metadata
 */
typedef struct {
    float latitude;     // Last latitude reading
    float longitude;    // Last longitude reading
    float speed;        // Last speed reading
    int year;           // Year when reading was taken
    int month;          // Month when reading was taken
    int day;            // Day when reading was taken
    int hour;           // Hour when reading was taken
    int min;            // Minute when reading was taken
    int sec;            // Second when reading was taken
    int cutback;        // Last cutback value
    int wave;           // Last wave data point
} GpsParams;

/* 
 * @struct NotiParams
 * @brief Structure to store notification parameters and metadata
 */
typedef struct {
    int year;           // Year of notification
    int month;          // Month of notification
    int day;            // Day of notification
    int hour;           // Hour of notification
    int min;            // Minute of notification
    int sec;            // Second of notification
    int cutback;        // Cutback angle
    int cutback_angle;  // Cutback angle data
    int wave;           // Wave data point
    int wave_distance;  // Wave distance data
    int session_duration; // Duration of session
} NotiParams;

/* 
 * @struct WaveParams
 * @brief Structure to store wave data and GPS metadata
 */
typedef struct {
    int year;                   // Year of wave data
    int month;                  // Month of wave data
    int day;                    // Day of wave data
    int hour;                   // Hour of wave data
    int min;                    // Minute of wave data
    int sec;                    // Second of wave data
    int ss_hour;                // Duration of Surf session(Hour)
    int ss_min;                 // Duration of Surf session(Minutes)
    int ss_sec;                 // Duration of Surf session(Seconds)
    int wave;                   // Wave data point
    int n_wave;                 // Number of waves surfed
    float wave_distance;        // Individual wave distance
    float max_wave_distance;    // Maximum wave distance
    float total_wave_distance;  // Total wave distance
    float speed;                // Speed recorded during wave
    float max_speed;            // Maximum speed during wave
    float temperature;          // Distance for this wave
    float gps_lat_start;
    float gps_long_start;
    float gps_lat_end;
    float gps_long_end;
    float latitude;
    float longitude;

} WaveParams;

/* 
 * @struct WaveGPSParams
 * @brief Structure to store wave data for GPS-related calculations
 */
typedef struct {
    int year;               // Year of wave data
    int month;              // Month of wave data
    int day;                // Day of wave data
    int hour;               // Hour of wave data
    int min;                // Minute of wave data
    int sec;                // Second of wave data
    int wave;               // Wave data point
    float wave_distance;    // Distance for this wave
    float total_wave_distance; // Total distance for all waves
    float speed;            // Speed recorded during wave
    float max_speed;        // Maximum speed during wave
    float latitude;         // Latitude during wave
    float longitude;        // Longitude during wave
} WaveGPSParams;

/* 
 * @struct WaveIMUParams
 * @brief Structure to store IMU data for wave detection
 */
typedef struct {
    int year;               // Year of IMU data
    int month;              // Month of IMU data
    int day;                // Day of IMU data
    int hour;               // Hour of IMU data
    int min;                // Minute of IMU data
    int sec;                // Second of IMU data
    int wave;               // Wave data point
    float acc_mag;          // Acceleration magnitude
    float smoothed_acc_3;   // Smoothed acceleration (3-point average)
    float smoothed_acc_4;   // Smoothed acceleration (4-point average)
    float smoothed_acc_5;   // Smoothed acceleration (5-point average)
    float smoothed_acc_6;   // Smoothed acceleration (6-point average)
} WaveIMUParams;

// Declaration of global parameter structures
extern WaterParams LastParams;      // Last temperature reading
extern DateTime LastDate;           // Last date and time recorded
extern ImuParams LastImuParams;     // Last IMU sensor data
extern GpsParams LastGpsParams;     // Last GPS data
extern NotiParams LastNotiParams;   // Last notification parameters
extern WaveParams LastWaveParams;   // Last wave data
extern WaveGPSParams LastWaveGPSParams; // Last wave data for GPS
extern WaveIMUParams LastWaveIMUParams; // Last wave data for IMU

// Callback function declarations to set DateTime structures
void setDateTime_G(void *datetime, struct tm *time);
void setDateTime_GPS(void *datetime, struct tm *time);
void setDateTime_IMU(void *datetime, struct tm *time);
void setDateTime_Wave(void *datetime, struct tm *time);
void setDateTime_WaveGPS(void *datetime, struct tm *time);
void setDateTime_WaveIMU(void *datetime, struct tm *time);

/**
 * @name LMPD_SYSTEM_handleActionT
 * @brief Receives temperature value from the sensor, stores it, and sends the value via Bluetooth SPP.
 * 
 * @param handle_ds Handler for DS18B20 sensor operation library.
 * @param param Pointer to the event parameters structure for Bluetooth SPP.
 * @return none
 */
void LMPD_SYSTEM_handleActionT(onewire_bus_handle_t handle_ds, esp_spp_cb_param_t *param);

/**
 * @brief Reads notification data from a CSV file and sends it via Bluetooth SPP.
 * 
 * @param handle_ds Handler for DS18B20 sensor operation library.
 * @param param Pointer to the event parameters structure for Bluetooth SPP.
 * @return none
 */
void LMPD_SYSTEM_handleActionNoti(onewire_bus_handle_t handle_ds, esp_spp_cb_param_t *param);

/**
 * @brief Controls SD card mode based on Bluetooth SPP communication.
 * 
 * @param handle_ds Handler for DS18B20 sensor operation library.
 * @param param Pointer to the event parameters structure for Bluetooth SPP.
 * @param sd_enable Boolean to enable or disable SD card.
 * @return none
 */
void LMPD_SYSTEM_handleActionB(onewire_bus_handle_t handle_ds, esp_spp_cb_param_t *param, bool sd_enable);

/**
 * @brief Reads wave data from a CSV file and sends it via Bluetooth SPP.
 * 
 * @param handle_ds Handler for DS18B20 sensor operation library.
 * @param param Pointer to the event parameters structure for Bluetooth SPP.
 * @return none
 */
void LMPD_SYSTEM_handleActionWave(onewire_bus_handle_t handle_ds, esp_spp_cb_param_t *param);

/**
 * @name LMPD_SYSTEM_Time
 * @brief Receives the timestamp from a smartphone via Bluetooth.
 * 
 * @param date Pointer to timestamp in "%d-%d-%d %d:%d" string format.
 * @return none
 */
void LMPD_SYSTEM_Time(char *date);

/**
 * @name LMPD_SYSTEM_save_parameters
 * @brief Saves parameters into pre-defined CSV headers.
 */
void LMPD_SYSTEM_save_parameters();

/**
 * @brief Saves IMU parameters to CSV.
 * 
 * @param params Pointer to IMU data structure.
 * @return none
 */
void save_imu_params_to_csv(const ImuParams *params);

/**
 * @brief Saves GPS parameters to CSV.
 * 
 * @param params Pointer to GPS data structure.
 * @return none
 */
void save_gps_params_to_csv(const GpsParams *params);

/**
 * @brief Saves temperature data to CSV.
 * 
 * @return none
 */
void save_tmp_params_to_csv();

/**
 * @brief Saves notification parameters to CSV.
 * 
 * @param params Pointer to notification data structure.
 * @return none
 */
void save_notification_params_to_csv(const NotiParams *params);

/**
 * @brief Saves wave parameters to CSV.
 * 
 * @param params Pointer to wave data structure.
 * @return none
 */
void save_wave_params_to_csv(const WaveParams *params);

/**
 * @brief Saves wave GPS parameters to CSV.
 * 
 * @param params Pointer to wave GPS data structure.
 * @return none
 */
void save_wavegps_params_to_csv(const WaveGPSParams *params);

/**
 * @brief Saves wave IMU parameters to CSV.
 * 
 * @param params Pointer to wave IMU data structure.
 * @return none
 */
void save_waveimu_params_to_csv(const WaveIMUParams *params);

/**
 * @name LMPD_SYSTEM_PM
 * @brief Controls the power to breakout sensor boards.
 * 
 * @param power_mode 1 to activate, 0 to deactivate the boards.
 * @return none
 */
void LMPD_SYSTEM_PM(bool power_mode);

/**
 * @brief Reads battery status and voltage, sending information over Bluetooth.
 * 
 * @param param Pointer to the event parameters structure for Bluetooth SPP.
 * @return none
 */
void LMPD_BATTERY_status(esp_spp_cb_param_t *param);

#endif /* MSG_HANDLER_H_ */
