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
#include <js_nvs.h>

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "js_db.h"
#include "js_fsm.h"
#include "js_util.h"
/*
==============================================================================================================================================
|         |0   |1   |2   |3   |4   |5   |6   |7   |8   |9   |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |21 |22 |23 |24 |25 |26 |27 |28 |29 |
|AltBeacon|0x1B|0xFF|MFG ID...|Code.....|Beacon-ID (20 Byte)................................................................|pwr|
|Eddy  UID|0x03|0x03|0xAA|0xFE|len |0x16|0xAA|0xFE|0x00|pwr |Namespace (10 byte)....................|Instance (6 byte)......|
|Eddy  EID|0x03|0x03|0xAA|0xFE|len |0x16|0xAA|0xFE|0x30|pwr |8-Byte (EID)...................|
==============================================================================================================================================
*/

/* Notes:
 * MFG ID = "The little endian representation of the beacon device manufacturer's company code as maintained by the Bluetooth SIG assigned numbers database"
 * Code: = 0xBEAC
 * PWR := calibrated RSSI at 1m
 * 0xXX: Manufacturer specific - ignore
*/
static char *TAG = "js_ble";

static uint8_t *own_eddystone_uid_data() {
    uint8_t *data = calloc(26, 1);
    // Common header
    data[0] = 0x03; data[1] = 0x03; data[2] = 0xAA; data[3] = 0xFE;
    data[4] = 0x15; data[5] = 0x16; data[6] = 0xAA; data[7] = 0xFE; data[8] = 0x00;
    // Transmission power in slot 9
    data[9] = -12 + (3 * esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV));
    // Fill in org-id
    memcpy(&data[10], js_nvs_ble_eddystone_my_org_id, 10);

    // Fill in instance-id
    memcpy(&data[20], js_nvs_ble_instance, 6);
    return data;
}

// Helper-Constants for faster comparison
static const uint32_t eddystone_first_4_bytes_in_little_endian = 0xFEAA0303;
static js_ble_beacon_t detect_beacon(uint8_t *rawData, size_t length) {
    if(length > 18) {
        uint8_t *data = rawData;
        ESP_LOGD(TAG, "Detecting Beacon data[0]=%02X data[1]=%02X data[2]=%02X data[3]=%02X data[4]=%02X data[5]=%02X data[6]=%02X data[7]=%02X data[8]=%02X",
                 data[0], data[1], data[2], data[3], data[4],data[5], data[6],data[7], data[8]);

        //if (data[0] == 0x03 && data[1] == 0x03 && data[2] == 0xAA && data[3] == 0xFE && data[5] == 0x16 && data[6] == 0xAA && data[7] == 0xFE) {
        // Note: Use 32-Bit operations for better performance
        if(*((uint32_t* )&data[0]) == eddystone_first_4_bytes_in_little_endian && data[5] == 0x16 && data[6] == 0xAA && data[7] == 0xFE) {
            if (data[8] == 0x00 && data[4] == 0x15 && length == 26) {
                ESP_LOGI(TAG, "Got: JS_BEACON_EDDYSTONE_UID - Length: %d", length);
                if(memcmp(js_nvs_ble_eddystone_my_org_id, &data[10], 10) == 0) {
                    // Check, if the instance-id needs just 1 or 2 bytes - Again use 32-Bit operations
                    //if(data[20] == 0x00 && data[21] == 0x00 && data[22] == 0x00 && data[23] == 0x00) {
                    if(*((uint32_t*) &data[20]) == 0) {
                        if(data[24] == 0) {
                            return JS_BEACON_EDDYSTONE_UID_MY_NETWORK_ONE_BYTE;
                        } else {
                            return JS_BEACON_EDDYSTONE_UID_MY_NETWORK_TWO_BYTES;
                        }
                    }
                    return JS_BEACON_EDDYSTONE_UID_MY_NETWORK;
                } else {
                    return JS_BEACON_EDDYSTONE_UID;
                }
            } else if (data[8] == 0x30 && data[4] == 0x11) {
                ESP_LOGI(TAG, "Got: JS_BEACON_EDDYSTONE_EID - Length: %d", length);
                return JS_BEACON_EDDYSTONE_EID;
            }
        } else {
            if ((length == 27 || length == 28) && (data[0] == 0x1B || data[0] == 0x1A) && (data[1] == 0xFF) && ((data[4] == 0xBE && data[5] == 0xAC) || (data[4] == 0x02 && data [5] == 0x15))) {
                ESP_LOGI(TAG, "Got: JS_BEACON_ALT_BEACON - Length: %d", length);
                return JS_BEACON_ALT_BEACON;
            }
        }

    }
    //ESP_LOGI(TAG, "Not a beacon");
    return  JS_BEACON_NONE;
}

// Sometimes ... data submitted in beacons contains the ADV-header having:
// Length == 0x02, Flags == 0x01 and Flags == 0x06 referring to the type of the advertisement
// Unfortunately, there is 3rd party equipment sending and not sending it.
// Possibly, some are buggy, so we're tolerant. Remove this header if it is present
// Wikipedia says: "Byte 0-2: Standard BLE Flags (Not necessary but standard)" (https://en.wikipedia.org/wiki/IBeacon)
static uint8_t header_offset(const uint8_t *data, size_t length) {
    if(length > 3 && data[0] == 0x02 && data[1] == 0x01 && data[2] == 0x06) {
        return 3;
    }
    return 0;
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


esp_err_t js_ble_scan_start() {
    ESP_LOGI(TAG, "Starting Scan");
    return esp_ble_gap_start_scanning(0);
}

static esp_ble_adv_params_t ble_adv_params = {
        .adv_int_min        = 0x00F0, // Min-Interval: 300 ms = 1.25 * 0x00F0
        .adv_int_max        = 0x0140, // Max-Interval: 400 ms = 1.25 * 0x0140
        .adv_type           = ADV_TYPE_NONCONN_IND,
        .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
        .channel_map        = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
esp_err_t js_ble_start_beacon() {
    ESP_LOGI(TAG, "Starting beacon");
    return esp_ble_gap_start_advertising(&ble_adv_params);
}
esp_err_t js_ble_stop_beacon() {
    ESP_LOGI(TAG, "Stopping beacon");
    return esp_ble_gap_stop_advertising();
}


void js_ble_scan_stop() {
    esp_ble_gap_stop_scanning();
}


static void js_ble_esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    esp_err_t err;

    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:{
            ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT");
            uint32_t duration = 0;
            esp_ble_gap_start_scanning(duration);
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_START_COMPLETE_EVT");
            if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Scan start failed: %s", esp_err_to_name(err));
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_START_COMPLETE_EVT");
            if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Adv start failed: %s", esp_err_to_name(err));
            }
            break;
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
            if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
                char mac[18];
                js_mac2strBLE(param->scan_rst.bda,mac);
//                ESP_LOGI(TAG,"MAC: %s",mac);
                uint8_t offset = header_offset(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len);
                uint8_t *data = &scan_result->scan_rst.ble_adv[offset];
                size_t length = scan_result->scan_rst.adv_data_len - offset;
                int8_t rssi = (int8_t)scan_result->scan_rst.rssi;

                js_ble_beacon_t type = detect_beacon(data, length);
                switch (type) {
                    case JS_BEACON_ALT_BEACON:
                        js_db_store_beacon(&data[6],rssi,type);
                        break;
                    case JS_BEACON_EDDYSTONE_EID: // NOLINT(bugprone-branch-clone)
                        js_db_store_beacon(&data[10],rssi,type);
                        break;
                    case JS_BEACON_EDDYSTONE_UID: // NOLINT(bugprone-branch-clone)
                        js_db_store_beacon(&data[10],rssi,type);
                        break;
                    case JS_BEACON_EDDYSTONE_UID_MY_NETWORK:
                        js_db_store_beacon(&data[20],rssi,type);
                        break;
                    case JS_BEACON_EDDYSTONE_UID_MY_NETWORK_TWO_BYTES:
                        js_db_store_beacon(&data[24],rssi,type);
                        break;
                    case JS_BEACON_EDDYSTONE_UID_MY_NETWORK_ONE_BYTE:
                        js_db_store_beacon(&data[25],rssi,type);
                        break;
                    default: // Also: JS_BEACON_NONE
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
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_AUTH_CMPL_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_AUTH_CMPL_EVT");
            break;
        case ESP_GAP_BLE_KEY_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_KEY_EVT");
            break;
        case ESP_GAP_BLE_SEC_REQ_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SEC_REQ_EVT");
            break;
        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PASSKEY_NOTIF_EVT");
            break;
        case ESP_GAP_BLE_PASSKEY_REQ_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PASSKEY_REQ_EVT");
            break;
        case ESP_GAP_BLE_OOB_REQ_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_OOB_REQ_EVT");
            break;
        case ESP_GAP_BLE_LOCAL_IR_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_LOCAL_IR_EVT");
            break;
        case ESP_GAP_BLE_LOCAL_ER_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_LOCAL_ER_EVT");
            break;
        case ESP_GAP_BLE_NC_REQ_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_NC_REQ_EVT");
            break;
        case ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT");
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT");
            break;
        case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_UPDATE_DUPLICATE_EXCEPTIONAL_LIST_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_UPDATE_DUPLICATE_EXCEPTIONAL_LIST_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_SET_CHANNELS_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SET_CHANNELS_EVT");
            break;
        case ESP_GAP_BLE_READ_PHY_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_READ_PHY_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_SET_PREFERED_DEFAULT_PHY_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SET_PREFERED_DEFAULT_PHY_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_SET_PREFERED_PHY_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SET_PREFERED_PHY_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_EXT_ADV_SET_REMOVE_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_EXT_ADV_SET_REMOVE_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_EXT_ADV_SET_CLEAR_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_EXT_ADV_SET_CLEAR_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_SET_PARAMS_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_SET_PARAMS_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_DATA_SET_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_START_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_START_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_STOP_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_STOP_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_CREATE_SYNC_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_CREATE_SYNC_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_SYNC_CANCEL_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_SYNC_CANCEL_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_SYNC_TERMINATE_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_SYNC_TERMINATE_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_ADD_DEV_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_ADD_DEV_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_REMOVE_DEV_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_REMOVE_DEV_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_CLEAR_DEV_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_CLEAR_DEV_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_SET_EXT_SCAN_PARAMS_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SET_EXT_SCAN_PARAMS_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_EXT_SCAN_START_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_EXT_SCAN_START_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_EXT_SCAN_STOP_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_EXT_SCAN_STOP_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_PREFER_EXT_CONN_PARAMS_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PREFER_EXT_CONN_PARAMS_SET_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_PHY_UPDATE_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PHY_UPDATE_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_EXT_ADV_REPORT_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_EXT_ADV_REPORT_EVT");
            break;
        case ESP_GAP_BLE_SCAN_TIMEOUT_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_TIMEOUT_EVT");
            break;
        case ESP_GAP_BLE_ADV_TERMINATED_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_TERMINATED_EVT");
            break;
        case ESP_GAP_BLE_SCAN_REQ_RECEIVED_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_REQ_RECEIVED_EVT");
            break;
        case ESP_GAP_BLE_CHANNEL_SELETE_ALGORITHM_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_CHANNEL_SELETE_ALGORITHM_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_REPORT_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_REPORT_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_SYNC_LOST_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_SYNC_LOST_EVT");
            break;
        case ESP_GAP_BLE_PERIODIC_ADV_SYNC_ESTAB_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_PERIODIC_ADV_SYNC_ESTAB_EVT");
            break;
        case ESP_GAP_BLE_EVT_MAX:
            ESP_LOGI(TAG, "ESP_GAP_BLE_EVT_MAX");
            break;
        default:
            ESP_LOGI(TAG, "Unhandled BLE event: %d",event );
            break;
    }
}
esp_err_t js_ble_receiver_register() {
    esp_err_t  err;
    if((err = esp_ble_gap_register_callback(js_ble_esp_gap_cb)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register callback %s", esp_err_to_name(err));
    }
    return err;
}


esp_err_t js_ble_scan_init() {
    JS_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    JS_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    JS_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    JS_ERROR_CHECK(esp_bluedroid_init());
    JS_ERROR_CHECK(esp_bluedroid_enable());
    JS_ERROR_CHECK(js_ble_receiver_register());
    JS_ERROR_CHECK(esp_ble_gap_set_scan_params(&ble_scan_params));
    JS_ERROR_CHECK(esp_ble_gap_config_adv_data_raw(own_eddystone_uid_data(),26));
    return ESP_OK;
}


inline size_t payload_length_of_type(js_ble_beacon_t type) {
    return type; // Easy for now: Length == Type
}
