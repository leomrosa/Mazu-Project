/**
 * @file gps_air.c - LIMPID project
 * @brief .c file to handle GPS module with UART interface
 * @details This driver provides initialization, data parsing, and handling for GPS data using the AIR530Z GPS module.
 * @version 1.0
 * @date 2024-10-13
 * 
 * authors Francisco Duarte
 *         Leonardo Rosa
 **/

#include "GPS_air.h"
#include <time.h>
#include "../LMPD_ch/MSG_handler.h"
#include "../LMPD_rtc/RTC_ds.h"

// Global GPS variables
RMCParsedData prevRMCData;  // Stores previous GPS data for distance calculations
double totalDistance = 0.0; // Tracks total distance traveled based on GPS data
int reading = 0;            // Counts number of GPS readings processed
bool rtc_initialized = false; // Flags if RTC has been initialized with GPS time


esp_err_t update_gps_orientation(float latitude, float longitude) {
    LastGpsParams.latitude = latitude;
    LastGpsParams.longitude = longitude;
    return ESP_OK;
}


esp_err_t adjustTimeForTimezone(struct tm* timeinfo, int offset_hours) {
    time_t rawtime = mktime(timeinfo);  // Converts struct tm to time_t format
    rawtime += offset_hours * 3600;     // Adds offset in seconds
    *timeinfo = *localtime(&rawtime);   // Updates struct tm with adjusted time
    return ESP_OK;
}


double convertToDecimalDegrees(double coord, char direction) {
    double degrees = floor(coord / 100.0);        // Extracts degrees from coordinate
    double minutes = coord - (degrees * 100.0);   // Extracts minutes from coordinate
    double decimal = degrees + minutes / 60.0;    // Converts to decimal degrees
    if (direction == 'S' || direction == 'W') {   // Adjusts for southern/western hemisphere
        decimal *= -1.0;
    }
    return decimal;
}


int parseRMC(const char *sentence, RMCParsedData *rmcData) {
    if (strncmp(sentence, GPS_RMC_PREFIX, strlen(GPS_RMC_PREFIX)) != 0) {
        return 0; // Returns 0 if the sentence is not an RMC sentence
    }

    char *token = strtok((char *)sentence, ",");  // Tokenizes the sentence by comma
    int fieldIndex = 0;

    while (token != NULL) {                       // Loops through each token in the sentence
        switch (fieldIndex) {
            case 1: strncpy(rmcData->time, token, sizeof(rmcData->time)); break;
            case 2: rmcData->status = token[0]; break;
            case 3: rmcData->latitude = strtod(token, NULL); break;
            case 4: rmcData->latitude = convertToDecimalDegrees(rmcData->latitude, token[0]); break;
            case 5: rmcData->longitude = strtod(token, NULL); break;
            case 6: rmcData->longitude = convertToDecimalDegrees(rmcData->longitude, token[0]); break;
            case 7: rmcData->speed = strtod(token, NULL); break;
            case 8: rmcData->course = strtod(token, NULL); break;
            case 9: strncpy(rmcData->date, token, sizeof(rmcData->date)); break;
        }
        token = strtok(NULL, ",");                // Moves to the next token
        fieldIndex++;
    }
    return 1; // Returns 1 on successful parse
}


double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000.0; // Earth radius in meters
    lat1 = lat1 * M_PI / 180.0;
    lon1 = lon1 * M_PI / 180.0;
    lat2 = lat2 * M_PI / 180.0;
    lon2 = lon2 * M_PI / 180.0;

    double dlat = lat2 - lat1;                 // Latitude difference
    double dlon = lon2 - lon1;                 // Longitude difference
    double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlon / 2) * sin(dlon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a)); // Haversine formula
    double distance = R * c;                   // Calculates the distance
    return distance;
}


esp_err_t convertGPSTimeDateToStructTM(const char* time, const char* date, struct tm* tm) {
    // Parses hours, minutes, seconds, day, month, and year from strings
    tm->tm_hour = (time[0] - '0') * 10 + (time[1] - '0');
    tm->tm_min = (time[2] - '0') * 10 + (time[3] - '0');
    tm->tm_sec = (time[4] - '0') * 10 + (time[5] - '0');
    tm->tm_mday = (date[0] - '0') * 10 + (date[1] - '0');
    tm->tm_mon = (date[2] - '0') * 10 + (date[3] - '0') - 1;
    tm->tm_year = (date[4] - '0') * 10 + (date[5] - '0') + 100; // Years since 1900
    return ESP_OK;
}


esp_err_t gps_initialize(void) {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_REF_TICK
    };

    uart_param_config(GPS_UART_NUM, &uart_config); // Applies UART configuration
    uart_set_pin(GPS_UART_NUM, GPS_TXD_PIN, GPS_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(GPS_UART_NUM, BUF_SIZE, BUF_SIZE, 0, NULL, 0);

    // Sends initial setup commands to the GPS module
    const char *nmea_sentence_rst_c_start = "$PGKC115,1,0,0,1*2A<CR><LF>";
    uart_write_bytes(GPS_UART_NUM, nmea_sentence_rst_c_start, strlen(nmea_sentence_rst_c_start));

    const char *nmea_sentence_GPS_GALLILEU = "$PGKC030,1,1*2C<CR><LF>";
    uart_write_bytes(GPS_UART_NUM, nmea_sentence_GPS_GALLILEU, strlen(nmea_sentence_GPS_GALLILEU));

    ESP_LOGI(TAG_GPS, "GPS Initialized");
    return ESP_OK;
}


esp_err_t air530z_task(void *pvParameters) {
    uint8_t data[BUF_SIZE];
    char currentSentence[BUF_SIZE];
    int sentenceIndex = 0;
    i2c_dev_t dev;
    memset(&dev, 0, sizeof(i2c_dev_t));

    while (1) {
        int len = uart_read_bytes(GPS_UART_NUM, data, BUF_SIZE - 1, 2000 / portTICK_PERIOD_MS);

        if (len > 0) {
            data[len] = 0;  // Null-terminate the data for processing as a string

            for (int i = 0; i < len; i++) {
                currentSentence[sentenceIndex++] = data[i];  // Build the sentence character by character

                if (data[i] == '\n') {  // End of sentence
                    currentSentence[sentenceIndex] = '\0';  // Null-terminate sentence
                    sentenceIndex = 0;  // Reset for next sentence

                    if (strncmp(currentSentence, GPS_RMC_PREFIX, strlen(GPS_RMC_PREFIX)) == 0) {
                        ESP_LOGI(TAG_GPS, "%s", currentSentence);

                        RMCParsedData rmcData;
                        if (parseRMC(currentSentence, &rmcData)) { // Parse RMC sentence
                            if (rmcData.status == 'A') {  // Valid GPS data
                                if (!rtc_initialized) {
                                    struct tm gpsTime;
                                    convertGPSTimeDateToStructTM(rmcData.time, rmcData.date, &gpsTime);

                                    // If year is valid, set RTC time from GPS data
                                    if (gpsTime.tm_year > 100) {
                                        adjustTimeForTimezone(&gpsTime, +1); 
                                        ds3231_set_time(&dev, &gpsTime); 
                                        rtc_initialized = true;
                                        ESP_LOGI(TAG_GPS, "RTC Initialized with GPS Time.\n");
                                    } else {
                                        ESP_LOGE(TAG_GPS, "Invalid GPS date, RTC not updated");
                                    }
                                }

                                // Calculate and add distance if it exceeds threshold
                                double distance = calculateDistance(
                                    prevRMCData.latitude, prevRMCData.longitude, 
                                    rmcData.latitude, rmcData.longitude
                                );
                                if (distance > 2.0) {
                                    totalDistance += distance;
                                }

                                // Update GPS data globally
                                update_gps_orientation(rmcData.latitude, rmcData.longitude);
                                LastWaveGPSParams.latitude = rmcData.latitude;
                                LastWaveGPSParams.longitude = rmcData.longitude;
                                LastWaveGPSParams.speed = rmcData.speed * 0.514444;  // Convert speed from knots to m/s

                                prevRMCData = rmcData;  // Save current reading for next comparison
                                reading++;  // Increment reading count
                            }
                        } else {
                            ESP_LOGI(TAG_GPS, "Failed to parse RMC sentence.\n");
                        }
                    }
                }
            }
        }
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500)); // Delays task to control reading interval
    }
    return ESP_OK;
}