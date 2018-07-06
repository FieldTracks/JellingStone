/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

/****************************************************************************
*
* This file is for iBeacon demo. It supports both iBeacon sender and receiver
* which is distinguished by macros IBEACON_SENDER and IBEACON_RECEIVER,
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

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_bt.h"
#include "esp_bt_main.h"

#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_device.h"



#include "ble.h"
#include "wifi.h"
#include "ntp.h"
#include "mqtt.h"
#include "util.h"
#include "esp_ibeacon_api.h"
#include "db.h"
#include "status.h"

#define BLE_SCAN_INTERVAL CONFIG_BLE_SCAN_INTERVAL

extern esp_ble_ibeacon_vendor_t vendor_config;

static char *MY_TAG ="main.c";

int cnt=0;

void dump_scanning_result(){
  char time_buf[128];
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  time_str(time_buf);
  char *message = db_dump_flush(time_buf);
  mqtt_publish(mac, message);
  ESP_LOGE(MY_TAG, "Got database: %s", message);

  free(message);
}

/** Initialize flash, Bluetooth for classic and BLE */
void init(){
  esp_err_t ret;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(MY_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM)) != ESP_OK) {
        ESP_LOGE(MY_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(MY_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(MY_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    char *dev_name = "JellingStone";
    esp_bt_dev_set_device_name(dev_name);

}

void app_main()
{
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK( ret );
  status_booting();
  /* Initialize NVS — it is used to store PHY calibration data */
  start_wifi();
  obtain_time();
  mqtt_start();
  init();
  ble_init();
  while(1){
    ESP_LOGI(MY_TAG, "Starting BLE - scan + beacon for %d seconds", BLE_SCAN_INTERVAL);
    ble_start();
    vTaskDelay(BLE_SCAN_INTERVAL * 1000 / portTICK_PERIOD_MS);
    ESP_LOGI(MY_TAG, "Stopping BLE ");
    ble_stop();
    //ESP_LOGI(MY_TAG, "BT Classic device scan for 5 seconds");
    //bt_classic_discover(5);
    //vTaskDelay(5 * 1010 / portTICK_PERIOD_MS);
    dump_scanning_result();
    ESP_LOGI(MY_TAG, "Done");
  }
}
