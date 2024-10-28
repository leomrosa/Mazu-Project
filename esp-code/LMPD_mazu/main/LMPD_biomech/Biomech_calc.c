/**
 * @file biomech_calc.c
 * @brief Source file for biomechanics and wave detection functions
 * @details Contains implementation of wave detection based on IMU and GPS data, including moving average calculation and cutback detection.
 *          The code includes real-time tracking of wave progress, start and end conditions, and total distance calculation.
 * @version 1.0
 * @date 2024-10-26
 * 
 * authors:
 * - Francisco Duarte
 * - Leonardo Rosa
 */
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

#include "Biomech_calc.h"
#include "../LMPD_ch/MSG_handler.h"
#include "../LMPD_gps/GPS_air.h"
#include "../LMPD_imu/IMU_mpu.h"
#include "../LMPD_rtc/RTC_ds.h"

// Global variables
bool wave_initiated_by_fall = false;  // Tracks if wave was initiated by a fall
bool in_wave = false;                 // Tracks if a wave is ongoing
int64_t wave_start_time = 0;          // Start time of wave
int64_t wave_confirm_time = 0;        // Time when wave was confirmed
int64_t wave_end_time = 0;            // End time of wave
int wave_count = 0;                   // Total wave count
double total_wave_distance = 0.0;     // Total distance covered by waves
double wave_distance = 0.0;           // Distance covered by waves
double max_wave_distance = 0.0;       // Max distance covered by waves

bool first_gps_reading = true;        // Track if it's the first GPS reading
int gps_reading_count = 0;            // Count of GPS readings

i2c_dev_t rtc_dev;                    // RTC device structure

// Arrays and indexes for smoothing acceleration data
int buffer_index = 0;
float acc_sum = 0;
int count = 0;

float acc_window_3[MAX_WINDOW_SIZE_3];
float acc_window_4[MAX_WINDOW_SIZE_4];
float acc_window_5[MAX_WINDOW_SIZE_5];
float acc_window_6[MAX_WINDOW_SIZE_6];

int acc_window_size_3 = 0, acc_window_size_4 = 0, acc_window_size_5 = 0, acc_window_size_6 = 0;
int acc_index_3 = 0, acc_index_4 = 0, acc_index_5 = 0, acc_index_6 = 0;

int consecutive_readings_above_threshold = 0;     // Count of consecutive readings above threshold
int consecutive_readings_below_fall_threshold = 0; // Count of consecutive readings below fall threshold

// Variables to track sudden acceleration changes
float last_acc_value = 0.0;
float acc_derivative = 0.0;

// Converts struct tm to milliseconds since epoch
int64_t convert_tm_to_ms(const struct tm *time) {
    return mktime((struct tm *)time) * 1000;
}

// Calculates moving average for a given window size
float calculate_moving_average(float new_value, float *window, int *index, int *size, int window_size) {
    window[*index] = new_value;
    *index = (*index + 1) % window_size;

    if (*size < window_size) {
        (*size)++;
    }

    float sum = 0.0;
    for (int i = 0; i < *size; i++) {
        sum += window[i];
    }

    return sum / *size;
}

// Detects waves using GPS speed and calculates distance
esp_err_t detect_waves_gps() {
    static int wave_in_progress = 0;     // Flag to indicate if wave is in progress
    static double max_speed = 0.0;       // Max speed during the wave
    static float prev_speed = 0.0;       // Previous speed value

    while (1) {
        // Get current GPS coordinates from LastWaveGPSParams
        float current_lat = LastWaveGPSParams.latitude;
        float current_lon = LastWaveGPSParams.longitude;
        LastWaveParams.latitude = current_lat;
        LastWaveParams.longitude = current_lon;

        float current_speed = LastWaveGPSParams.speed;

        // Start new wave if above speed threshold and no wave is in progress
        if (current_speed > SPEED_THRESHOLD && wave_in_progress == 0) {
            wave_in_progress = 1;
            LastWaveParams.wave = 1;
            LastWaveGPSParams.wave = 1;

            LastWaveParams.gps_lat_start = current_lat;
            LastWaveParams.gps_long_start = current_lon;

            max_speed = current_speed;  // Initialize max speed
        }

        // Update max speed and wave data during wave
        if (wave_in_progress == 1) {
            if (current_speed > max_speed) {
                max_speed = current_speed;
            }

            LastWaveParams.speed = current_speed;
            LastWaveParams.max_speed = max_speed;

            LastWaveGPSParams.speed = current_speed;
            LastWaveGPSParams.max_speed = max_speed;
        }

        // End wave after two consecutive readings below threshold
        if (current_speed <= SPEED_END_THRESHOLD && prev_speed <= SPEED_END_THRESHOLD && wave_in_progress == 1) {
            wave_in_progress = 0;
            LastWaveParams.wave = 2;
            LastWaveGPSParams.wave = 2;

            LastWaveParams.gps_lat_end = current_lat;
            LastWaveParams.gps_long_end = current_lon;

            // Calculate wave distance
            LastWaveParams.wave_distance = calculateDistance(
                LastWaveParams.gps_lat_start, LastWaveParams.gps_long_start,
                LastWaveParams.gps_lat_end, LastWaveParams.gps_long_end
            );

            wave_distance = LastWaveParams.wave_distance;

            LastWaveParams.total_wave_distance += LastWaveParams.wave_distance;
            LastWaveGPSParams.wave_distance = LastWaveParams.wave_distance;
            LastWaveGPSParams.total_wave_distance = LastWaveParams.total_wave_distance;
            wave_count++;
            LastWaveParams.n_wave = wave_count;

            if(wave_distance > max_wave_distance){

                LastWaveParams.total_wave_distance = wave_distance;
                max_wave_distance = wave_distance;

            }



            max_speed = 0.0;  // Reset max speed after wave ends
        }

        prev_speed = current_speed; // Update previous speed for next iteration

        vTaskDelay(200 / portTICK_PERIOD_MS);  // Small delay for controlled reading intervals
    }
    return ESP_OK;
}


/*

void detect_waves_imu() {
    vector_t va, vg, vm;
    float acc_magnitude, smoothed_acc_3, smoothed_acc_4, smoothed_acc_5, smoothed_acc_6;
        // Variáveis de controle para leituras consecutivas
    int consecutive_readings_above_threshold = 0;
    int consecutive_readings_below_fall_threshold = 0;
    int consecutive_readings_below_acc_end_threshold = 0;  // Nova variável
    int consecutive_readings_above_fall_end_threshold = 0; // Nova variável

    while (1) {
        // Ler dados do IMU
        ESP_ERROR_CHECK(get_accel_gyro_mag(&va, &vg, &vm));

        // Transformar os dados para a orientação do dispositivo
        transform_accel_gyro(&va);
        transform_accel_gyro(&vg);
        transform_mag(&vm);

        // Calcular a magnitude da aceleração
        acc_magnitude = sqrt(va.x * va.x + va.y * va.y + va.z * va.z);

        // Dentro do loop principal
        smoothed_acc_3 = calculate_moving_average(acc_magnitude, acc_window_3, &acc_index_3, &acc_window_size_3, MAX_WINDOW_SIZE_3);
        smoothed_acc_4 = calculate_moving_average(acc_magnitude, acc_window_4, &acc_index_4, &acc_window_size_4, MAX_WINDOW_SIZE_4);
        smoothed_acc_5 = calculate_moving_average(acc_magnitude, acc_window_5, &acc_index_5, &acc_window_size_5, MAX_WINDOW_SIZE_5);
        smoothed_acc_6 = calculate_moving_average(acc_magnitude, acc_window_6, &acc_index_6, &acc_window_size_6, MAX_WINDOW_SIZE_6);

        printf("smoothed_acc_3 = %.2f\n", smoothed_acc_3);
        printf("smoothed_acc_4 = %.2f\n", smoothed_acc_4);
        printf("smoothed_acc_5 = %.2f\n", smoothed_acc_5);
        printf("smoothed_acc_6 = %.2f\n", smoothed_acc_6);

        // Armazenar os valores suavizados
        LastWaveIMUParams.acc_mag = acc_magnitude;
        LastWaveIMUParams.smoothed_acc_3 = smoothed_acc_3;
        LastWaveIMUParams.smoothed_acc_4 = smoothed_acc_4;
        LastWaveIMUParams.smoothed_acc_5 = smoothed_acc_5;
        LastWaveIMUParams.smoothed_acc_6 = smoothed_acc_6;

        // Verificar se a aceleração está acima do limiar de onda
        if (smoothed_acc_5 > ACC_THRESHOLD) {
            consecutive_readings_above_threshold++;

            // Se houver 3 leituras consecutivas acima do threshold, inicie a onda
            if (!in_wave && consecutive_readings_above_threshold >= 3) {
                in_wave = true;
                wave_initiated_by_fall = false;  // Onda iniciada por aumento
                ESP_LOGI(TAG_BioMech, "Onda detectada por aumento com 3 leituras consecutivas!");
                LastWaveIMUParams.wave = 1;  // Onda detectada
            }
        } else {
            // Reiniciar o contador se o valor da aceleração não for maior que o limiar
            consecutive_readings_above_threshold = 0;
        }

        // Verificar se a aceleração está abaixo do limiar de queda (detecção de onda por queda)
        if (smoothed_acc_5 < FALL_THRESHOLD) {
            consecutive_readings_below_fall_threshold++;

            // Se houver 3 leituras consecutivas abaixo do threshold, inicie a onda por queda
            if (!in_wave && consecutive_readings_below_fall_threshold >= 3) {
                in_wave = true;
                wave_initiated_by_fall = true;  // Onda iniciada por queda
                ESP_LOGI(TAG_BioMech, "Onda detectada por queda com 3 leituras consecutivas!");
                LastWaveIMUParams.wave = 1;  // Onda detectada
            }
        } else {
            // Reiniciar o contador se o valor da aceleração não for menor que o limiar de queda
            consecutive_readings_below_fall_threshold = 0;
        }

        // Verificar o fim da onda
        if (in_wave) {
            if (!wave_initiated_by_fall) {
                // Fim da onda por aumento seguido de queda
                if (smoothed_acc_5 < ACC_END_THRESHOLD) {
                    consecutive_readings_below_acc_end_threshold++;

                    if (consecutive_readings_below_acc_end_threshold >= 3) {
                        wave_count++;
                        ESP_LOGI(TAG_BioMech, "Onda finalizada! Contagem: %d, Distância: %.2f metros", wave_count, total_wave_distance);

                        LastWaveIMUParams.wave = 2;

                        in_wave = false;
                        consecutive_readings_below_acc_end_threshold = 0;
                        total_wave_distance = 0.0;
                    }
                } else {
                    consecutive_readings_below_acc_end_threshold = 0;
                }
            } else {
                // Fim da onda por queda seguida de subida
                if (smoothed_acc_5 > FALL_END_THRESHOLD) {
                    consecutive_readings_above_fall_end_threshold++;

                    if (consecutive_readings_above_fall_end_threshold >= 3) {
                        wave_count++;
                        ESP_LOGI(TAG_BioMech, "Onda finalizada! Contagem: %d, Distância: %.2f metros", wave_count, total_wave_distance);

                        LastWaveIMUParams.wave = 2;

                        in_wave = false;
                        consecutive_readings_above_fall_end_threshold = 0;
                        total_wave_distance = 0.0;
                    }
                } else {
                    consecutive_readings_above_fall_end_threshold = 0;
                }
            }
        }

        ESP_LOGI(TAG_BioMech, "Fora da Onda: Onda struct %.6f; Tamanho de onda calculado: %.6f\n", 
                 LastNotiParams.wave_distance, total_wave_distance);

        vTaskDelay(100 / portTICK_PERIOD_MS); // Pequeno delay para evitar leituras contínuas
    }
}*/


/*
// Function to detect and process a cutback maneuver
void process_cutback() {
    double start_lat = 0.0, start_lon = 0.0;
    double end_lat = 0.0, end_lon = 0.0;
    bool cutback_detected = false;
    double initial_yaw = 0.0;
    double total_distance = 0.0;
    int reading = 0;

    while (1) {
        // Collect IMU data
        vector_t va, vg, vm;
        ESP_ERROR_CHECK(get_accel_gyro_mag(&va, &vg, &vm));
        transform_accel_gyro(&va);
        transform_accel_gyro(&vg);
        transform_mag(&vm);
        ahrs_update(DEG2RAD(vg.x), DEG2RAD(vg.y), DEG2RAD(vg.z),
                    va.x, va.y, va.z,
                    vm.x, vm.y, vm.z);

        // Get Euler angles (heading, pitch, roll)
        float heading, pitch, roll;
        ahrs_get_euler_in_degrees(&heading, &pitch, &roll);

        // Check if we detect a significant change in direction (potential start of a cutback)
        if (!cutback_detected && fabs(heading - initial_yaw) > CUTBACK_ANGLE_THRESHOLD) {
            cutback_detected = true;
            start_lat = WaveRMCData.latitude;
            start_lon = WaveRMCData.longitude;
            initial_yaw = heading;

            LastImuParams.cutback = 1;
            LastGpsParams.cutback = 1;
            ESP_LOGI(TAG_BioMech, "Cutback initiated at heading: %.2f°", heading);
        }

        // Check if the cutback has stabilized (end of the maneuver)
        if (cutback_detected && fabs(heading - initial_yaw) < HEADING_STABILIZATION_THRESHOLD) {
            end_lat = WaveRMCData.latitude;
            end_lon = WaveRMCData.longitude;

            double distance = calculateDistance(start_lat, start_lon, end_lat, end_lon);
            double angle = heading - initial_yaw;

            // Ensure valid movement has occurred
            if (distance > MINIMUM_DISTANCE) {
                ESP_LOGI(TAG_BioMech, "Cutback detected!");
                ESP_LOGI(TAG_BioMech, "Distance: %.2f meters", distance);
                ESP_LOGI(TAG_BioMech, "Angle: %.2f degrees", angle);
                LastImuParams.cutback = 2;
                LastGpsParams.cutback = 2;
                LastNotiParams.cutback = 1;
                LastNotiParams.cutback_angle = angle;
            }

            cutback_detected = false;
            initial_yaw = heading;
        } else {
            LastImuParams.cutback = -1;
            LastGpsParams.cutback = -1;
            ESP_LOGI(TAG_BioMech, "Cutback not confirmed !");

        }

        

        // Pause for data collection
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}*/