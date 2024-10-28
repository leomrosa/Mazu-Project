
/**
 * @file RTC_ds.h - LIMPID project
 * @brief .h file to handle DS3231 RTC module
 * @details This header defines the functions and structures used to initialize
 *          and read data from the DS3231 RTC over the I2C protocol
 * @version 1.0
 * @date 2024-10-24
 * 
 * @authors Francisco Duarte
 *          Leonardo Rosa
 *          
 **/

#ifndef DS3231_TEST_H_
#define DS3231_TEST_H_

#include <time.h>
#include <stdint.h>
#include "i2cdev.h"
#include "../LMPD_ch/MSG_handler.h"

/**
 * @brief Function pointer type for setting date and time.
 * 
 * This type defines a callback function used to fill in the datetime structure
 * after reading the date and time from the DS3231.
 */
typedef void (*DateTimeSetter)(void *datetime, struct tm *time);

/**
 * @brief Initializes the DS3231 RTC device.
 * 
 * Sets up the I2C device structure and initializes the DS3231 RTC by setting the initial date and time.
 * 
 * @param[in] dev Pointer to the I2C device structure.
 * 
 * @return
 *         - ESP_OK on success.
 *         - ESP_FAIL if initialization fails.
 */
esp_err_t ds3231_init(i2c_dev_t *dev);

/**
 * @brief Task to read the date and time from DS3231 RTC.
 * 
 * This FreeRTOS task reads the current date and time from the DS3231 RTC, populates a datetime structure via a callback,
 * and prints the date and time.
 * 
 * @param[in] pvParameters Pointer to the I2C device parameters.
 * @param[in] datetime Pointer to the datetime structure.
 * @param[in] setDateTime Callback function to set datetime.
 * 
 * @return
 *         - ESP_OK on success.
 *         - ESP_FAIL if the task fails to read time.
 */
esp_err_t ds3231_task(void *pvParameters, void *datetime, DateTimeSetter setDateTime);

#endif /* DS3231_TEST_H_ */
