#include <string.h>
#include "esp_log.h"
#include "cJSON.h"
#include "util.h"
#include "nvs.h"
#include "esp_timer.h"

#include "mqtt.h"
#include "esp_system.h"
#include "esp_bt_device.h"

#include "esp_ibeacon_api.h"
#include "esp_wifi.h"

void mqtt_status_transmit(){
  wifi_ap_record_t ap_info;
  esp_wifi_sta_get_ap_info(&ap_info);
  const uint8_t *mac = esp_bt_dev_get_address();

  char mac_str[18];
  mac2strBLE(mac, mac_str);

  cJSON *status = cJSON_CreateObject();

  cJSON_AddItemToObject(status, "mac", cJSON_CreateString(mac_str));

  cJSON_AddItemToObject(status, "maj", cJSON_CreateNumber(get_ble_major()));
  cJSON_AddItemToObject(status, "min", cJSON_CreateNumber(get_ble_minor()));

  cJSON_AddItemToObject(status, "int", cJSON_CreateNumber(get_ble_scan_interval()));
  cJSON_AddItemToObject(status, "up", cJSON_CreateNumber(esp_timer_get_time() / 1000 / 1000));
  cJSON_AddItemToObject(status, "bat", cJSON_CreateNumber(-1));

  // Wifi data
  char bssid[48];
  mac2str(ap_info.bssid, bssid);
  cJSON_AddItemToObject(status, "bs", cJSON_CreateString(bssid));
  cJSON_AddItemToObject(status, "ch", cJSON_CreateNumber(ap_info.primary));
  cJSON_AddItemToObject(status, "rx", cJSON_CreateNumber(ap_info.rssi));

  char *message = cJSON_Print(status);
  cJSON_Delete(status);
  mqtt_publish_msg("status",message);

  free(message);
}
