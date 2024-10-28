/**
 * @file gps_air.h - LIMPID project
 * @brief Header file for GPS module functions
 * @details Contains declarations for initializing and handling GPS data with the AIR530Z module.
 * @version 1.0
 * @date 2024-10-13
 * 
 * authors Francisco Duarte
 *         Leonardo Rosa
 **/

#ifndef AIR530Z_TASK_H
#define AIR530Z_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include "ds3231.h"

// UART and GPS settings
#define GPS_UART_NUM UART_NUM_1
#define GPS_TXD_PIN  17  
#define GPS_RXD_PIN  16  
#define BUF_SIZE (1024)
#define GPS_RMC_PREFIX "$GNRMC" 

static const char *TAG_GPS = "Gps";

// RMC data structure
typedef struct {
    char time[11];      // Time in HHMMSS.SSS format (UTC)
    char date[7];       // Date in DDMMYY format (UTC)
    char status;        // 'A' for valid data, 'V' for invalid
    double latitude;    // Latitude in decimal degrees
    double longitude;   // Longitude in decimal degrees
    double speed;       // Speed over ground in knots
    double course;      // Course over ground in degrees
} RMCParsedData;

/**
 * @brief Updates the GPS orientation data.
 * 
 * Updates the global structure with the current latitude and longitude
 * received from GPS data.
 * 
 * @param[in] latitude Latitude in decimal degrees.
 * @param[in] longitude Longitude in decimal degrees.
 * 
 * @return
 *         - ESP_OK on success.
 */
esp_err_t update_gps_orientation(float latitude, float longitude);

/**
 * @brief Converts GPS coordinates to decimal degrees.
 * 
 * Converts latitude and longitude from NMEA format to decimal degrees,
 * applying necessary adjustments based on the direction (N/S or E/W).
 * 
 * @param[in] coord GPS coordinate in NMEA format.
 * @param[in] direction 'N', 'S', 'E', or 'W' indicating the direction.
 * 
 * @return
 *         - The coordinate in decimal degrees.
 */
double convertToDecimalDegrees(double coord, char direction);

/**
 * @brief Parses RMC sentence from GPS.
 * 
 * Parses an NMEA RMC sentence and extracts relevant data into the RMCParsedData structure.
 * 
 * @param[in] sentence The NMEA sentence to parse.
 * @param[out] rmcData Pointer to the structure to store parsed data.
 * 
 * @return
 *         - 1 on successful parse.
 *         - 0 if the sentence is not an RMC sentence.
 */
int parseRMC(const char *sentence, RMCParsedData *rmcData);

/**
 * @brief Calculates distance between two coordinates.
 * 
 * Uses the Haversine formula to calculate the distance between two points specified
 * by latitude and longitude.
 * 
 * @param[in] lat1 Latitude of the first point in decimal degrees.
 * @param[in] lon1 Longitude of the first point in decimal degrees.
 * @param[in] lat2 Latitude of the second point in decimal degrees.
 * @param[in] lon2 Longitude of the second point in decimal degrees.
 * 
 * @return
 *         - The distance between the points in meters.
 */
double calculateDistance(double lat1, double lon1, double lat2, double lon2);

/**
 * @brief Converts GPS time and date to struct tm.
 * 
 * Converts the GPS-provided time and date (in strings) to a struct tm format for
 * easier manipulation and potential RTC updates.
 * 
 * @param[in] time Time string in HHMMSS.SSS format.
 * @param[in] date Date string in DDMMYY format.
 * @param[out] tm Pointer to struct tm to store the converted time and date.
 * 
 * @return
 *         - ESP_OK on success.
 */
esp_err_t convertGPSTimeDateToStructTM(const char* time, const char* date, struct tm* tm);

/**
 * @brief Initializes the GPS module with UART configuration.
 * 
 * Configures UART for communication with the AIR530Z GPS module and
 * sends initial commands to set up the GPS.
 * 
 * @return
 *         - ESP_OK on success.
 */
esp_err_t gps_initialize(void);

/**
 * @brief GPS task for reading and processing GPS data.
 * 
 * Main task function for reading data from the GPS module, processing RMC sentences,
 * and updating relevant parameters.
 * 
 * @param[in] pvParameters Task parameters.
 * 
 * @return
 *         - ESP_OK on success.
 */
esp_err_t air530z_task(void *pvParameters);

/**
 * @brief Adjusts time for a specified timezone.
 * 
 * Adjusts the provided time structure to account for timezone offset in hours.
 * 
 * @param[in,out] timeinfo Pointer to the struct tm containing the time.
 * @param[in] offset_hours Timezone offset in hours.
 * 
 * @return
 *         - ESP_OK on success.
 */
esp_err_t adjustTimeForTimezone(struct tm* timeinfo, int offset_hours);

// Global GPS variables
extern RMCParsedData prevRMCData;
extern double totalDistance;
extern int reading;
extern bool rtc_initialized;

#endif // AIR530Z_TASK_H
