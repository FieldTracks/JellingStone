/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#include <esp_err.h>
#include "js_ble.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "js_db.h"

/*
==============================================================================================================================================
|         |0   |1   |2   |3   |4   |5   |6   |7   |8   |9   |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |21 |22 |23 |24 |25 |26 |27 |28 |29 |
|AltBeacon|0x02|0x01|0x06|len |0xFF|MFG ID...|Code.....|Beacon-ID (20 Byte).............................................................|pwr|
|Eddy  UID|0x02|0x01|0x06|0xAA|0xFE|len |0x16|0xAA|0xFE|0x00|pwr|Namespace (10 byte)....................|Instance (6 byte)......|
|Eddy  EID|0x02|0x01|0x06|0xAA|0xFE|len |0x16|0xAA|0xFE|0x30|pwr|8-Byte (EID)...................|
==============================================================================================================================================
*/

/* Notes:
 * len := 0x1B or 0x1A
 * MFG ID = "The little endian representation of the beacon device manufacturer's company code as maintained by the Bluetooth SIG assigned numbers database"
 * Code: = 0xBEAC
 * PWR := calibrated RSSI at 1m
*/
static char *TAG = "js_ble";

static js_ble_beacon_t detect_beacon(const uint8_t *data, size_t length) {
    ESP_LOGD(TAG, "Got frame - length %d", length);
    if(length > 18) {
        ESP_LOGD(TAG, "Checking flags: data[0]=%02x data[1]=%02x data[2]=%02x", data[0], data[1], data[2]);
        if (data[0] == 0x02 && data[1] == 0x02 && data[2] == 0x06) { // Flags match
            // Check for Eddystone
            ESP_LOGD(TAG, "Checking for Eddystone data[3]=%02x data[4]=%02x data[6]=%02x data[7]=%02x data[8]=%02x",data[3], data[4], data[6], data[7], data[8]);
            if (data[3] == 0xAA && data[4] == 0xFE && data[6] == 0x16 && data[7] == 0xAA && data[8] == 0xFE) {
                ESP_LOGD(TAG, "Eddystone - checking type  data[9]=%02x",data[9]);
                if (data[9] == 0x00) {
                    ESP_LOGD(TAG, "Got: JS_BEACON_EDDISTONE_UID");
                    return JS_BEACON_EDDISTONE_UID;
                } else if (data[9] == 0x30) {
                    ESP_LOGD(TAG, "Got: JS_BEACON_EDDISTONE_EID");
                    return JS_BEACON_EDDISTONE_EID;
                }
            }else {
                ESP_LOGD(TAG, "Checking for ALtBeacon data[7]=%02x data[8]=%02x",data[7],data[8]);
                if ((data[7] == 0xBE && data[8] == 0xAC) || (data[7] == 0x52 && data [8] == 0x04)) {
                    ESP_LOGD(TAG, "Got: JS_BEACON_ALT_BEACON");
                    return JS_BEACON_ALT_BEACON;
                }
            }
        }
    }
    ESP_LOGD(TAG, "Not a beacon");
    return  JS_BEACON_NONE;
}

// Receiver configuration
static esp_ble_scan_params_t ble_scan_params = {
        .scan_type              = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_duplicate         = BLE_SCAN_DUPLICATE_ENABLE,
        /*  Scan interval. This is defined as the time interval from when the Controller started its last LE scan until it begins the subsequent LE scan.
        Range: 0x0004 to 0x4000 Default: 0x0010 (10 ms), Time = N * 0.625 msec,  Time Range: 2.5 msec to 10.24 seconds */
        .scan_interval          = 0x09c0, // 4 seconds
        /* The duration of the LE scan. LE_Scan_Window shall be less than or equal to LE_Scan_Interval
           Range: 0x0004 to 0x4000 Default: 0x0010 (10 ms) Time = N * 0.625 msec Time Range: 2.5 msec to 10240 msec */
        .scan_window            = 0x0943, // 3.8 s
};


void js_ble_scan_start(uint32_t durationInSeconds) {
    esp_ble_gap_start_scanning(durationInSeconds);
}

void js_ble_scan_stop() {
    esp_ble_gap_stop_scanning();

}


static void js_ble_esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    esp_err_t err;

    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:{
            uint32_t duration = 0;
            esp_ble_gap_start_scanning(duration);
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Scan start failed: %s", esp_err_to_name(err));
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Adv start failed: %s", esp_err_to_name(err));
            }
            break;
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
            if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
                js_ble_beacon_t type = detect_beacon(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len);
                switch (type) {
                    case JS_BEACON_ALT_BEACON: {
                        js_db_process_altbeacon(&scan_result->scan_rst.ble_adv[9],
                                                &scan_result->scan_rst.ble_adv[25],
                                                (int8_t) scan_result->scan_rst.ble_adv[29],
                                                (int8_t) scan_result->scan_rst.rssi);
                        break;
                    }
                    case JS_BEACON_EDDISTONE_EID:
                        js_db_process_eddy_eid(&scan_result->scan_rst.ble_adv[9],
                                               (int8_t) scan_result->scan_rst.ble_adv[10],
                                               (int8_t) scan_result->scan_rst.rssi);
                        break;
                    case JS_BEACON_EDDISTONE_UID:
                        js_db_process_eddy_uid(&scan_result->scan_rst.ble_adv[9],
                                               &scan_result->scan_rst.ble_adv[21],
                                               (int8_t) scan_result->scan_rst.ble_adv[10],
                                               (int8_t) scan_result->scan_rst.rssi);
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(TAG, "Scan stop failed: %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(TAG, "Stop scan successfully");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if ((err = param->adv_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(TAG, "Adv stop failed: %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(TAG, "Stop adv successfully");
            }
            break;

        default:
            break;
    }
}
static void js_ble_alt_beacon_register() {
    esp_err_t  err;
    if((err = esp_ble_gap_register_callback(js_ble_esp_gap_cb)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register callback %s", esp_err_to_name(err));
    }
}

void js_ble_scan_init() {
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();
    js_ble_alt_beacon_register();
    esp_ble_gap_set_scan_params(&ble_scan_params);
}

