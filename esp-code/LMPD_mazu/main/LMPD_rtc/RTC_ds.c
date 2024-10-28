
/**
 * @file RTC_ds.c - LIMPID project
 * @brief .c file to handle DS3231 RTC module
 * @details This driver provides initialization and task handling for reading
 *          time data from the DS3231 RTC over the I2C protocol
 * @version 1.0
 * @date 2024-10-24
 * 
 * @authors Francisco Duarte
 *          Leonardo Rosa
 *          
 **/

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ds3231.h>
#include <string.h>
#include "RTC_ds.h"


esp_err_t ds3231_init(i2c_dev_t *dev) {
    memset(dev, 0, sizeof(i2c_dev_t));  // Clear the device structure to ensure clean initialization

    // Attempt to set the initial time on the DS3231 RTC
    esp_err_t err = ds3231_set_time(dev, &time);
    if (err != ESP_OK) {
        printf("Failed to initialize DS3231 RTC\n");  // Log error if setting time fails
        return ESP_FAIL;
    }
    return ESP_OK;
}

 
esp_err_t ds3231_task(void *pvParameters, void *datetime, DateTimeSetter setDateTime) {
    float temp;
    struct tm time;

    // Attempt to read time from the DS3231 RTC; return ESP_FAIL if unsuccessful
    esp_err_t err = ds3231_get_time((i2c_dev_t *)pvParameters, &time);
    if (err != ESP_OK) {
        printf("Could not get time\n");  // Log error if unable to read time
        return ESP_FAIL;
    }

    // Use the callback function to populate the datetime structure with the obtained time
    setDateTime(datetime, &time);

    // Log the retrieved date and time in YYYY-MM-DD HH:MM:SS format for debugging purposes
    printf("RTC %04d-%02d-%02d %02d:%02d:%02d\n",
           time.tm_year + 1900, time.tm_mon + 1,
           time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);

    return ESP_OK;
}