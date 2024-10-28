#ifndef BLT_SPP_H
#define BLT_SPP_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_spp_api.h"
#include "esp_system.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include <stdio.h>
#include <string.h>
#include "sys/time.h"

#include "../LMPD_ch/MSG_handler.h"



#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SPP_TAG "SPP_ACCEPTOR_DEMO"
#define SPP_SERVER_NAME "SPP_SERVER"


// Define a macro to choose between Mazu and Varuna
#define DEVICE_CHOICE 0 // Set to 0 for Mazu, 1 for Varuna

#if DEVICE_CHOICE == 0
    #define EXAMPLE_DEVICE_NAME "MAZU2"
#elif DEVICE_CHOICE == 1
    #define EXAMPLE_DEVICE_NAME "VARUNA5"
#else
    #error "Invalid device choice. Must be either 0 (Mazu) or 1 (Varuna)."
#endif

#define SPP_SHOW_DATA 0
#define SPP_SHOW_SPEED 1
#define SPP_SHOW_MODE SPP_SHOW_DATA    /*Choose show mode: show data or speed*/

extern const esp_spp_mode_t esp_spp_mode;
extern const bool esp_spp_enable_l2cap_ertm;
extern struct timeval time_new, time_old;
extern long data_num;
extern const esp_spp_sec_t sec_mask;
extern const esp_spp_role_t role_slave;

extern bool bluetooth_connected;
extern bool sd_mode;



/**
 * @brief Converts Bluetooth device address to a string.
 * 
 * This function converts a Bluetooth device address to a string format.
 * 
 * @param bda Pointer to the Bluetooth device address.
 * @param str Pointer to the destination string.
 * @param size Size of the destination string buffer.
 * @return Pointer to the destination string if successful, otherwise NULL.
 */
char *bda2str(uint8_t * bda, char *str, size_t size);

/**
 * @brief Prints the data transmission speed.
 * 
 * This function calculates and prints the data transmission speed.
 */
void print_speed(void);

/**
 * @brief Callback function for Bluetooth Serial Port Profile (SPP) events.
 * 
 * This function serves as a callback for handling Bluetooth SPP events.
 * 
 * @param event The event type.
 * @param param Pointer to the event parameters structure.
 */
void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

/**
 * @brief Callback function for Bluetooth GAP events.
 * 
 * This function serves as a callback for handling Bluetooth GAP events.
 * 
 * @param event The event type.
 * @param param Pointer to the event parameters structure.
 */
void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

#endif /* BLUETOOTH_FUNCTIONS_H */
