/** 
 * @file MSG_handler.c - LIMPID project
 * @brief .c file to handle system operations
 * @details This driver provides functions to handle responses using Bluetooth SPP
 * @version 1.0
 * @date 2024-10-24
 * 
 * authors Francisco Duarte
 *         Leonardo Rosa
 **/

#include "MSG_handler.h"
#include "../LMPD_sen/SEN_ds.h"
#include "../LMPD_rtc/RTC_ds.h"

// Global error result
esp_err_t result;

// Definition of global parameter structures
WaterParams LastParams;
DateTime LastDate;
ImuParams LastImuParams;
GpsParams LastGpsParams;
NotiParams LastNotiParams;
WaveParams LastWaveParams;
WaveGPSParams LastWaveGPSParams;
WaveIMUParams LastWaveIMUParams;

static esp_adc_cal_characteristics_t adc2_chars;  // ADC calibration

#define DS18B20_PIN GPIO_NUM_4
#define MAX_LINE_LENGTH 500
char line_buffer[MAX_LINE_LENGTH];

// Callback function to set DateTime for generic structure
void setDateTime_G(void *datetime, struct tm *time) {
    DateTime *dt_G = (DateTime *)datetime;
    dt_G->year = time->tm_year + 1900;
    dt_G->month = time->tm_mon + 1;
    dt_G->day = time->tm_mday;
    dt_G->hour = time->tm_hour;
    dt_G->min = time->tm_min;
    dt_G->sec = time->tm_sec;
}

// Callback function to set DateTime for GPS data
void setDateTime_GPS(void *datetime, struct tm *time) {
    GpsParams *dt_gps = (GpsParams *)datetime;
    dt_gps->year = time->tm_year + 1900;
    dt_gps->month = time->tm_mon + 1;
    dt_gps->day = time->tm_mday;
    dt_gps->hour = time->tm_hour;
    dt_gps->min = time->tm_min;
    dt_gps->sec = time->tm_sec;
}

// Callback function to set DateTime for IMU data
void setDateTime_IMU(void *datetime, struct tm *time) {
    ImuParams *dt_IMU = (ImuParams *)datetime;
    dt_IMU->year = time->tm_year + 1900;
    dt_IMU->month = time->tm_mon + 1;
    dt_IMU->day = time->tm_mday;
    dt_IMU->hour = time->tm_hour;
    dt_IMU->min = time->tm_min;
    dt_IMU->sec = time->tm_sec;
}

// Callback function to set DateTime for Wave data
void setDateTime_Wave(void *datetime, struct tm *time) {
    WaveParams *dt_Wave = (WaveParams *)datetime;
    dt_Wave->year = time->tm_year + 1900;
    dt_Wave->month = time->tm_mon + 1;
    dt_Wave->day = time->tm_mday;
    dt_Wave->hour = time->tm_hour;
    dt_Wave->min = time->tm_min;
    dt_Wave->sec = time->tm_sec;
}

// Callback function to set DateTime for WaveGPS data
void setDateTime_WaveGPS(void *datetime, struct tm *time) {
    WaveGPSParams *dt_WaveGPS = (WaveGPSParams *)datetime;
    dt_WaveGPS->year = time->tm_year + 1900;
    dt_WaveGPS->month = time->tm_mon + 1;
    dt_WaveGPS->day = time->tm_mday;
    dt_WaveGPS->hour = time->tm_hour;
    dt_WaveGPS->min = time->tm_min;
    dt_WaveGPS->sec = time->tm_sec;
}

// Callback function to set DateTime for WaveIMU data
void setDateTime_WaveIMU(void *datetime, struct tm *time) {
    WaveIMUParams *dt_WaveIMU = (WaveIMUParams *)datetime;
    dt_WaveIMU->year = time->tm_year + 1900;
    dt_WaveIMU->month = time->tm_mon + 1;
    dt_WaveIMU->day = time->tm_mday;
    dt_WaveIMU->hour = time->tm_hour;
    dt_WaveIMU->min = time->tm_min;
    dt_WaveIMU->sec = time->tm_sec;
}

// Handles enabling/disabling SD card via Bluetooth (SPP)
void LMPD_SYSTEM_handleActionB(onewire_bus_handle_t handle_ds, esp_spp_cb_param_t *param, bool sd_enable)
{
    LMPD_SYSTEM_PM(POWER_MODE_ON);  // Enable power mode

    char sppSdEnable[6] = "";  // Buffer to store the state of sd_enable

    // Formats the string based on sd_enable state
    if (sd_enable) {
        sprintf(sppSdEnable, "trueB");
    } else {
        sprintf(sppSdEnable, "falseB");
    }

    // Sends sd_enable state via Bluetooth
    esp_spp_write(param->write.handle, strlen(sppSdEnable), (uint8_t *)sppSdEnable);
}

// Handles temperature data and sends it over Bluetooth (SPP)
void LMPD_SYSTEM_handleActionT(onewire_bus_handle_t handle_ds, esp_spp_cb_param_t *param)
{
    LMPD_SYSTEM_PM(POWER_MODE_ON);
    char sppVoltageT[4] = "";
    LastParams.Temperature = ds18b20_readTemperature(handle_ds);

    sprintf(sppVoltageT, "%.1fT", LastParams.Temperature);
    esp_spp_write(param->write.handle, strlen(sppVoltageT), (uint8_t *)sppVoltageT);
}

// Reads notification data from a CSV file and sends it via Bluetooth
void LMPD_SYSTEM_handleActionNoti(onewire_bus_handle_t handle_ds, esp_spp_cb_param_t *param)
{
    LMPD_SYSTEM_PM(POWER_MODE_ON);

    LMPD_device_read_csv_line_by_line(MOUNT_POINT"/noti.csv", line_buffer, MAX_LINE_LENGTH);
    printf("Read line: %s\n", line_buffer);

    esp_spp_write(param->write.handle, strlen(line_buffer), (uint8_t *)line_buffer);
}

// Reads wave data from a CSV file and sends it via Bluetooth
void LMPD_SYSTEM_handleActionWave(onewire_bus_handle_t handle_ds, esp_spp_cb_param_t *param)
{
    LMPD_SYSTEM_PM(POWER_MODE_ON);

    // Lê a linha do arquivo CSV
    esp_err_t read_status = LMPD_device_read_csv_line_by_line(MOUNT_POINT "/output.csv", line_buffer, MAX_LINE_LENGTH);
    
    // Verifica se houve erro ou se a linha está vazia
    if (read_status == ESP_ERR_NOT_FOUND || strlen(line_buffer) == 0) {
        // Envia mensagem de término e apaga o arquivo
        const char *error_message = "End";
        esp_spp_write(param->write.handle, strlen(error_message), (uint8_t *)error_message);
        ESP_LOGE("LMPD_SYSTEM", "No valid data available in the CSV file.");
        
        // Apaga o conteúdo do arquivo após a leitura completa
        LMPD_device_clear_output_file();
        
        return; // Para de enviar informações
    }

    // Exibe e envia a linha lida
    printf("Read line: %s\n", line_buffer);
    esp_spp_write(param->write.handle, strlen(line_buffer), (uint8_t *)line_buffer);
}

// Saves headers to various CSV files to ensure the format is prepared
void LMPD_SYSTEM_save_parameters()
{
    // Different header definitions for various CSV files
    const char *headers_imu[] = { "Day", "Hour", "Heading", "Pitch", "Roll", "Cutback", "Wave" };
    const char *headers_gps[] = { "Day", "Hour", "Latitude", "Longitude", "Speed", "Cutback", "Wave" };
    const char *headers_tmp[] = { "Day", "Hour", "Temperature" };
    const char *headers_n[] = { "Day", "Hour", "Cutback", "Cutback Angle", "Wave", "Wave Distance" };
    const char *headers_w[] = { "Day", "Hour","Surf Session Duration", "Wave","Number of Waves Surfed", "Wave Distance", "Max Wave Distance", "Total Wave Distance", "Speed", "Max Speed", "Teperature" };
    const char *headers_w_gps[] = { "Day", "Hour", "Wave", "Wave Distance", "Total Wave Distance", "Speed", "Max Speed" };
    const char *headers_w_imu[] = { "Day", "Hour", "Wave", "Acc Mag", "Moving Avg Acc MAG(3)", "Moving Avg Acc MAG(4)", "Moving Avg Acc MAG(5)", "Moving Avg Acc MAG(6)" };

    save_headers_to_csv(MOUNT_POINT"/imu.csv", headers_imu, sizeof(headers_imu) / sizeof(headers_imu[0]));
    save_headers_to_csv(MOUNT_POINT"/gps.csv", headers_gps, sizeof(headers_gps) / sizeof(headers_gps[0]));
    save_headers_to_csv(MOUNT_POINT"/tmp.csv", headers_tmp, sizeof(headers_tmp) / sizeof(headers_tmp[0]));
    save_headers_to_csv(MOUNT_POINT"/noti.csv", headers_n, sizeof(headers_n) / sizeof(headers_n[0]));
    save_headers_to_csv(MOUNT_POINT"/wave.csv", headers_w, sizeof(headers_w) / sizeof(headers_w[0]));
    save_headers_to_csv(MOUNT_POINT"/wave_gps.csv", headers_w_gps, sizeof(headers_w_gps) / sizeof(headers_w_gps[0]));
    save_headers_to_csv(MOUNT_POINT"/wave_imu.csv", headers_w_imu, sizeof(headers_w_imu) / sizeof(headers_w_imu[0]));
}

// Saves temperature data to CSV
void save_tmp_params_to_csv() {
    ds3231_task(NULL, &LastDate, setDateTime_G);  // Updates DateTime
    LastParams.Temperature = ds18b20_readTemperature(handle_ds);

    FILE* f = fopen(MOUNT_POINT "/tmp.csv", "a");
    if (f == NULL) {
        printf("Failed to open file for writing\n");
        return;
    }

    fprintf(f, "%04d-%02d-%02d; %02d:%02d:%02d;%.6f\n",
            LastDate.year, LastDate.month, LastDate.day,
            LastDate.hour, LastDate.min, LastDate.sec,
            LastParams.Temperature);

    fclose(f);
    printf("Data saved to tmp.csv successfully.\n\n");
}

// Saves IMU data to CSV
void save_imu_params_to_csv(const ImuParams *params) {
    FILE* f = fopen(MOUNT_POINT "/imu.csv", "a");
    if (f == NULL) {
        printf("Failed to open file for writing\n");
        return;
    }

    fprintf(f, "%04d-%02d-%02d; %02d:%02d:%02d;%.6f;%.6f;%.6f;%02d;%02d\n",
            params->year, params->month, params->day,
            params->hour, params->min, params->sec,
            params->heading, params->pitch, params->roll,
            params->cutback, params->wave);

    fclose(f);
    LastImuParams.cutback = 0;
    printf("Data saved to imu.csv successfully.\n\n");
}

// Saves GPS data to CSV
void save_gps_params_to_csv(const GpsParams *params) {
    FILE* f = fopen(MOUNT_POINT "/gps.csv", "a");
    if (f == NULL) {
        printf("Failed to open file for writing\n");
        return;
    }

    fprintf(f, "%04d-%02d-%02d; %02d:%02d:%02d;%.6f;%.6f;%.6f;%02d;%02d\n",
            params->year, params->month, params->day,
            params->hour, params->min, params->sec,
            params->latitude, params->longitude,
            params->speed, params->cutback, params->wave);

    fclose(f);
    LastGpsParams.cutback = 0;
    printf("Data saved to gps.csv successfully.\n\n");
}

// Saves notification data to CSV
void save_notification_params_to_csv(const NotiParams *params) {
    ds3231_task(NULL, &LastNotiParams, setDateTime_G); // Updates DateTime
    FILE* f = fopen(MOUNT_POINT "/noti.csv", "a");
    if (f == NULL) {
        printf("Failed to open file for writing\n");
        return;
    }

    fprintf(f, "%04d-%02d-%02d; %02d:%02d:%02d; %.6f;%.6f;%.6f;%.6f;\n",
            LastNotiParams.year, LastNotiParams.month, LastNotiParams.day,
            LastNotiParams.hour, LastNotiParams.min, LastNotiParams.sec,
            params->cutback, params->cutback_angle,
            params->wave, params->wave_distance);

    fclose(f);
    LastNotiParams.cutback = 0;
    LastNotiParams.wave = 0;
    LastNotiParams.wave_distance = 0;
    printf("Data saved to noti.csv successfully.\n\n");
}

// Saves wave data to CSV
void save_wave_params_to_csv(const WaveParams *params) {
    ds3231_task(NULL, &LastWaveParams, setDateTime_G); // Updates DateTime
    FILE* f = fopen(MOUNT_POINT "/wave.csv", "a");
    if (f == NULL) {
        printf("Failed to open file for writing\n");
        return;
    }

    fprintf(f, "%04d-%02d-%02d; %02d:%02d:%02d; %02d:%02d:%02d; %d; %d;%.6f;%.6f;%.6f;%.6f;%.6f;%.6f;\n",
            LastWaveParams.year, LastWaveParams.month, LastWaveParams.day,
            LastWaveParams.hour, LastWaveParams.min, LastWaveParams.sec,
            params->ss_hour, params->ss_min, params->ss_sec,
            params->wave, params->n_wave, params->wave_distance, params->max_wave_distance, params->total_wave_distance,
            params->speed, params->max_speed, params->temperature);

    fclose(f);
    LastWaveParams.wave_distance = 0;
    LastWaveParams.ss_hour =0, LastWaveParams.ss_min =0, LastWaveParams.ss_sec =0;
    printf("Data saved to wave.csv successfully.\n\n");
}

// Saves WaveGPS data to CSV
void save_wavegps_params_to_csv(const WaveGPSParams *params) {
    ds3231_task(NULL, &LastWaveGPSParams, setDateTime_G); // Updates DateTime
    FILE* f = fopen(MOUNT_POINT "/wave_gps.csv", "a");
    if (f == NULL) {
        printf("Failed to open file for writing\n");
        return;
    }

    fprintf(f, "%04d-%02d-%02d; %02d:%02d:%02d; %d;%.6f;%.6f;%.6f;%.6f;\n",
            LastWaveGPSParams.year, LastWaveGPSParams.month, LastWaveGPSParams.day,
            LastWaveGPSParams.hour, LastWaveGPSParams.min, LastWaveGPSParams.sec,
            params->wave, params->wave_distance, params->total_wave_distance,
            params->speed, params->max_speed);

    fclose(f);
    LastWaveGPSParams.wave_distance = 0;
    printf("Data saved to wave_gps.csv successfully.\n\n");
}

// Saves WaveIMU data to CSV
void save_waveimu_params_to_csv(const WaveIMUParams *params) {
    ds3231_task(NULL, &LastWaveIMUParams, setDateTime_G); // Updates DateTime
    FILE* f = fopen(MOUNT_POINT "/wave_imu.csv", "a");
    if (f == NULL) {
        printf("Failed to open file for writing\n");
        return;
    }

    fprintf(f, "%04d-%02d-%02d; %02d:%02d:%02d; %d;%.6f;%.6f;%.6f;%.6f;%.6f;\n",
            LastWaveIMUParams.year, LastWaveIMUParams.month, LastWaveIMUParams.day,
            LastWaveIMUParams.hour, LastWaveIMUParams.min, LastWaveIMUParams.sec,
            params->wave, params->acc_mag, params->smoothed_acc_3, 
            params->smoothed_acc_4, params->smoothed_acc_5, params->smoothed_acc_6);

    fclose(f);
    printf("Data saved to wave_imu.csv successfully.\n\n");
}

// Sets the system time from a given date string
void LMPD_SYSTEM_Time(char *date)
{      
    sscanf(date, "%d-%d-%d %d:%d", &LastDate.year, &LastDate.month, &LastDate.day, &LastDate.hour, &LastDate.min);

    /* Last response sent -> Power off breakout boards */
    LMPD_SYSTEM_PM(POWER_MODE_OFF);
}

// Power mode control for peripheral devices
void LMPD_SYSTEM_PM(bool power_mode)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;      // Disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;            // Set as output mode
    io_conf.pin_bit_mask = (1ULL << GPIO_PIN_POWER); // Set the GPIO pin bit mask
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;   // Disable pull-up resistor
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down resistor

    gpio_config(&io_conf); // Configure GPIO pin settings

    if (power_mode) {
        gpio_set_level(GPIO_PIN_POWER, 1);  // Activate GPIO pin
        printf("GPIO pin %d activated\n", GPIO_PIN_POWER);
    } else {
        gpio_set_level(GPIO_PIN_POWER, 0);  // Deactivate GPIO pin
        printf("GPIO pin %d deactivated\n", GPIO_PIN_POWER);
    }
}

// Reads and sends battery status and voltage over Bluetooth
void LMPD_BATTERY_status(esp_spp_cb_param_t *param)
{
    char status_charge[30] = ""; 
    char value_charge[10] = ""; 
    char combined_message[50] = "";  // Combined message buffer

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_PIN_STAT);
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    gpio_config(&io_conf);  // Configure GPIO pin

    // Check battery charging status
    if (gpio_get_level(GPIO_PIN_STAT)) {
        strcpy(status_charge, "Completed/unplaced");   
    } else {
        strcpy(status_charge, "Energizing");   
    }

    // Configures ADC settings for voltage read
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc2_config_channel_atten(ADC2_CHANNEL_6, ADC_ATTEN_DB_11));

    int raw_value;
    esp_err_t err = adc2_get_raw(ADC2_CHANNEL_6, ADC_WIDTH_BIT_12, &raw_value);
    if (err != ESP_OK) {
        ESP_LOGE(BAT_TAG, "ADC2 read error: %d", err);
        return;
    }

    uint16_t voltage_mv = esp_adc_cal_raw_to_voltage(raw_value, &adc2_chars); // Convert raw value to mV

    sprintf(value_charge, "%umV", voltage_mv); // Format voltage as string

    sprintf(combined_message, "Status: %s, Voltage: %s", status_charge, value_charge); // Combine charge status and voltage

    esp_spp_write(param->write.handle, strlen(combined_message), (uint8_t*)combined_message); // Send data via Bluetooth
    ESP_LOGI(BAT_TAG, "Battery Status and Voltage: %s", combined_message);  // Log the battery status and voltage
}
