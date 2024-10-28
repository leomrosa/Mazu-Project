/**
 * @file ROM_msd.h
 * @brief Header file for handling microSD card module using SPI protocol.
 * @details This driver provides functions for initializing, reading, writing, and managing the SD card.
 * @version 1.0
 * @date 2024-03-24
 * 
 * @authors
 * Francisco Duarte
 * Leonardo Rosa
 */

#ifndef ROM_MSD_H
#define ROM_MSD_H

#include "esp_err.h"
#include "esp_log.h" // Include the ESP logging header
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_types.h" // Include the header for sdmmc_card_t

#define MOUNT_POINT "/sdcard"

// Pin assignments can be set in menuconfig, see "SD SPI Example Configuration" menu.
// You can also change the pin assignments here by changing the following 4 lines.
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   15

#define KEY_LENGTH 10

#define SD_CARD_TAG "SDcard"

extern sdmmc_host_t host; // Declare the host variable as extern
extern sdmmc_card_t *card;
extern bool regist;
// Declaração da variável global
extern int sd_enable;

/**
 * @brief Registers the device by generating and storing a random key.
 * 
 * @param filename The name of the file to store the registration information.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t LMPD_device_register(const char *filename);

/**
 * @brief Writes a parameter and its corresponding float data to a file.
 * 
 * @param filename The name of the file.
 * @param parameter The parameter name.
 * @param data The float data value.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t LMPD_device_writing(const char *filename, char *parameter, float data);

esp_err_t LMPD_device_read_csv_line_by_line(const char *filename, char *line_buffer, size_t buffer_size);

/**
 * @brief Writes a newline character to a file to add a space.
 * 
 * @param filename The name of the file.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t LMPD_device_writing_space(const char *filename);

/**
 * @brief Writes a parameter and its corresponding string data to a file.
 * 
 * @param filename The name of the file.
 * @param parameter The parameter name.
 * @param data The string data value.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t LMPD_device_writing_string(const char *filename, char *parameter, char* data);

/**
 * @brief Reads a block of data from a CSV file on the SD card.
 * 
 * @param filename The name of the CSV file.
 * @param block_buffer The buffer to store the read block of data.
 * @param buffer_size The size of the block buffer.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t LMPD_device_read_block(const char *filename, char *block_buffer, size_t buffer_size);

/**
 * @brief Writes a parameter and its corresponding float data with specified precision to a file.
 * 
 * @param filename The name of the file.
 * @param parameter The parameter name.
 * @param data The float data value.
 * @param precision The precision of the float data.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t LMPD_device_writing_f(const char *filename, char *parameter, float data, int precision);

/**
 * @brief Initializes the SD card and performs registration.
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t sd_card_init(void);

/**
 * @brief Configures the SD card settings and mounts the filesystem.
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t sd_card_config(void);

/**
 * @brief Renames a file on the SD card.
 * 
 * @param old_filename The current name of the file.
 * @param new_filename The new name for the file.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t rename_file(const char *old_filename, const char *new_filename);

/**
 * @brief Writes data to a file on the SD card.
 * 
 * @param filename The name of the file.
 * @param data The data to write.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t sd_card_write(const char *filename, const char *data);

/**
 * @brief Reads data from a file on the SD card.
 * 
 * @param filename The name of the file.
 * @param buffer The buffer to store the read data.
 * @param buffer_size The size of the buffer.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t sd_card_read(const char *filename, char *buffer, size_t buffer_size);

/**
 * @brief Reads the last valid line from the wave.csv file and copies it to output.csv.
 * Then clears the contents of wave.csv.
 * 
 * @return ESP_OK on success, ESP_FAIL if there is an error opening/reading the files.
 */
esp_err_t LMPD_device_copy_last_valid_line();

/**
 * @brief Reads the last valid line from a CSV file.
 * 
 * This function opens the specified file, navigates to the end, and moves back
 * to find the beginning of the last line with valid information.
 * 
 * @param filename The full path to the CSV file to be read.
 * @param line_buffer The output buffer where the last line will be stored.
 * @param buffer_size The size of the output buffer, which should be large enough to store a complete line.
 * 
 * @return ESP_OK on success, ESP_FAIL if there is an error opening or reading the file.
 */
esp_err_t LMPD_device_read_last_valid_line(const char *filename, char *line_buffer, size_t buffer_size);

/**
 * @brief Clears the contents of the wave.csv file, deleting all its lines.
 * 
 * @return ESP_OK on success, ESP_FAIL if there is an error opening the file.
 */
esp_err_t LMPD_device_clear_file();
esp_err_t LMPD_device_clear_output_file();
esp_err_t LMPD_device_clear_wave_file();

void save_headers_to_csv(const char *file_path, const char *headers[], size_t header_count);

#endif // ROM_MSD_H
