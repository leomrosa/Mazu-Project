/**
 * @file BLT_spp.c - LIMPID project
 * @brief .c file for BLT driver
 * @details This driver provides functions to initialize and configure the SPP bluetooth protocol.
 * @version 1.0
 * @date 2023-07-24
 * 
 * @author Francisco Duarte
 *         Leonardo Rosa
 *         
 **/


#include "BLT_spp.h"
#include <ctype.h>  // For isdigit function

bool sd_mode = false;


char *bda2str(uint8_t * bda, char *str, size_t size)
{
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

void print_speed(void)
{
    float time_old_s = time_old.tv_sec + time_old.tv_usec / 1000000.0;
    float time_new_s = time_new.tv_sec + time_new.tv_usec / 1000000.0;
    float time_interval = time_new_s - time_old_s;
    float speed = data_num * 8 / time_interval / 1000.0;
    ESP_LOGI(SPP_TAG, "speed(%fs ~ %fs): %f kbit/s" , time_old_s, time_new_s, speed);
    data_num = 0;
    time_old.tv_sec = time_new.tv_sec;
    time_old.tv_usec = time_new.tv_usec;
}

void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    char bda_str[18] = {0};

    switch (event) {
        case ESP_SPP_INIT_EVT:
            if (param->init.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
                esp_spp_start_srv(sec_mask, role_slave, 0, SPP_SERVER_NAME);
            } else {
                ESP_LOGE(SPP_TAG, "ESP_SPP_INIT_EVT status:%d", param->init.status);
            }
            break;
            
        case ESP_SPP_DISCOVERY_COMP_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
            break;

        case ESP_SPP_OPEN_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT");
            break;

        case ESP_SPP_CLOSE_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_CLOSE_EVT status:%d handle:%d close_by_remote:%d", 
                     param->close.status, param->close.handle, param->close.async);
            bluetooth_connected = false;
            ESP_LOGI(SPP_TAG, "BLT_OFF");
            break;

        case ESP_SPP_START_EVT:
            if (param->start.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT handle:%d sec_id:%d scn:%d", param->start.handle, param->start.sec_id,
                         param->start.scn);
                esp_bt_dev_set_device_name(EXAMPLE_DEVICE_NAME);
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            } else {
                ESP_LOGE(SPP_TAG, "ESP_SPP_START_EVT status:%d", param->start.status);
            }
            break;

        case ESP_SPP_CL_INIT_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT");
            break;

        case ESP_SPP_DATA_IND_EVT:
            if (!bluetooth_connected) {
                ESP_LOGW(SPP_TAG, "SPP não conectado; ignorando evento de dados.");
                return;
            }

            ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT len:%d handle:%d", param->data_ind.len, param->data_ind.handle);
            if (param->data_ind.len < 300) {
                esp_log_buffer_hex("", param->data_ind.data, param->data_ind.len);
            }

            char *received_data = (char *)param->data_ind.data;

            switch (received_data[0]) {
                case 'T':
                    ESP_LOGI(SPP_TAG, "Received 'T' on DS");
                    LMPD_SYSTEM_handleActionT(handle_ds, param);
                    break;

                case 'R':
                    ESP_LOGI(SPP_TAG, "Received Notifications");
                    LMPD_SYSTEM_handleActionNoti(handle_ds, param);
                    break;

                case 'W':
                    ESP_LOGI(SPP_TAG, "Received Notifications");
                    LMPD_SYSTEM_handleActionWave(handle_ds, param);
                    break;


                case 'B':
                    ESP_LOGI(SPP_TAG, "Received 'B' for sd_enable");
                    LMPD_SYSTEM_handleActionB(handle_ds, param, sd_enable);
                    break;

                case 'M':
                    ESP_LOGI(SPP_TAG, "Received 'SD' - Store in SD command'");
                    sd_mode = true;
                    break;

                case 'N':
                    ESP_LOGI(SPP_TAG, "Received 'DB' - Store in Firebase'");
                    sd_mode = false;
                    break;

                default:
                    ESP_LOGI(SPP_TAG, "Entering Default");
                    break;
            }

            if (strncmp(received_data, "DATE:", 5) == 0) {
                char *date_string = received_data + 5;
                ESP_LOGI(SPP_TAG, "Received date: %s", date_string);
                LMPD_SYSTEM_Time(date_string);
                LMPD_SYSTEM_save_parameters(sd_mode);
            }

            memset(received_data, 0, param->data_ind.len);
            break;

        case ESP_SPP_CONG_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
            break;

        case ESP_SPP_WRITE_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_WRITE_EVT");
            break;

        case ESP_SPP_SRV_OPEN_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT status:%d handle:%d, rem_bda:[%s]", 
                     param->srv_open.status, param->srv_open.handle, 
                     bda2str(param->srv_open.rem_bda, bda_str, sizeof(bda_str)));
            bluetooth_connected = true;
            ESP_LOGI(SPP_TAG, "BLT_ON");
            gettimeofday(&time_old, NULL);
            break;

        case ESP_SPP_SRV_STOP_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_STOP_EVT");
            break;

        case ESP_SPP_UNINIT_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_UNINIT_EVT");
            break;

        default:
            break;
    }
}


void  esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    char bda_str[18] = {0};

    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(SPP_TAG, "authentication success: %s bda:[%s]", param->auth_cmpl.device_name,
                     bda2str(param->auth_cmpl.bda, bda_str, sizeof(bda_str)));
        } else {
            ESP_LOGE(SPP_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT:{
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(SPP_TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(SPP_TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif

    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d bda:[%s]", param->mode_chg.mode,
                 bda2str(param->mode_chg.bda, bda_str, sizeof(bda_str)));
        break;

    default: {
        ESP_LOGI(SPP_TAG, "event: %d", event);
        break;
    }
    }
    return;
}
