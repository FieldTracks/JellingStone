/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COLi along with JellingStone.
    If not, please contact info@fieldtracks.org

    Based on esp-idf example code, distributed as public domain and CC0
*/



/****************************************************************************
*
* This file is for iBeacon APIs. It supports both iBeacon encode and decode.
*
* iBeacon is a trademark of Apple Inc. Before building devices which use iBeacon technology,
* visit https://developer.apple.com/ibeacon/ to obtain a license.
*
****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs_flash.h"
#include "db.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_ibeacon_api.h"
#include "esp_log.h"
//#include "common/ringbuf.h"

static const char* DEMO_TAG = "ibeacon.c";
extern esp_ble_ibeacon_vendor_t vendor_config;

///Declare static functions
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

typedef struct rb_elem{
  esp_bd_addr_t addr;
  int rssi;
  uint64_t timestmp;
} rb_elem_t;

//static rb_elem_t buffer[127];
////static buffer_pos = 0;

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x0500,
    .scan_window            = 0x0450
};

static esp_ble_adv_params_t ble_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_NONCONN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};


static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:{
        esp_ble_gap_start_advertising(&ble_adv_params);
        break;
    }
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second, 0 means scan permanently
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(DEMO_TAG, "Scan start failed");
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //adv start complete event to indicate adv start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(DEMO_TAG, "Adv start failed");
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        int remoteRssi = 0;
        uint16_t major = 0;
        uint16_t minor = 0;
        uint8_t proximity_uuid[DB_UUID_LENGTH_IN_BYTE];
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        uint8_t isBeacon = 0;
        if (esp_ble_is_ibeacon_packet(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len)){
            esp_ble_ibeacon_t *ibeacon_data = (esp_ble_ibeacon_t*)(scan_result->scan_rst.ble_adv);
            //esp_log_buffer_hex("IBEACON_DEMO: Device address:", scan_result->scan_rst.bda, ESP_BD_ADDR_LEN );
            //esp_log_buffer_hex("IBEACON_DEMO: Proximity UUID:", ibeacon_data->ibeacon_vendor.proximity_uuid, ESP_UUID_LEN_128);
            remoteRssi = ibeacon_data->ibeacon_vendor.measured_power;
            major = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.major);
            minor = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.minor);
            memcpy(proximity_uuid,ibeacon_data->ibeacon_vendor.proximity_uuid,DB_UUID_LENGTH_IN_BYTE);
            isBeacon = 1;
        } else {
          // ESP_LOGE(DEMO_TAG, "Kein iBeacon");
          memset(proximity_uuid, 0, DB_UUID_LENGTH_IN_BYTE);
          memcpy(proximity_uuid,scan_result->scan_rst.bda,6);
        }
        /*ESP_LOGI(DEMO_TAG, "---------- BT device found----------");
        esp_log_buffer_hex("IBEACON_DEMO: Device address:", scan_result->scan_rst.bda, BD_ADDR_LEN );
        ESP_LOGI(DEMO_TAG, "RSSI of packet:%d dbm", scan_result->scan_rst.rssi);*/
        db_add(scan_result->scan_rst.rssi,remoteRssi,major,minor,proximity_uuid,isBeacon);
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(DEMO_TAG, "Scan stop failed");
        }
        else {
            ESP_LOGI(DEMO_TAG, "Stop scan successfully");
        }
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(DEMO_TAG, "Adv stop failed");
        }
        else {
            ESP_LOGI(DEMO_TAG, "Stop adv successfully");
        }
        break;

    default:
        break;
    }
}


void ble_ibeacon_appRegister(void)
{
    esp_err_t status;
    ESP_LOGI(DEMO_TAG, "register callback");

    //register the scan callback function to the gap module
    if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
        ESP_LOGE(DEMO_TAG, "gap register error, error code = %x", status);
        return;


    }

}
/** Scan for 5 secods, dump result, continue Scanning */
void ble_start(){
  esp_ble_ibeacon_t ibeacon_adv_data;
  esp_err_t status = esp_ble_config_ibeacon_data (&vendor_config, &ibeacon_adv_data);
  if (status != ESP_OK) {
      ESP_LOGE(DEMO_TAG, "%s enable ibeacon failed: %s\n", __func__, esp_err_to_name(status));
      return;
  }


  esp_ble_gap_config_adv_data_raw((uint8_t*)&ibeacon_adv_data, sizeof(ibeacon_adv_data));
  esp_ble_gap_start_scanning(0);

}
void ble_stop(){
  esp_ble_gap_stop_scanning();
}

void ble_init()
{
    ble_ibeacon_appRegister();
    esp_ble_gap_set_scan_params(&ble_scan_params);

}
