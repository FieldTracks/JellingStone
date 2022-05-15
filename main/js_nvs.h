/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
#include "nvs_flash.h"
#include <stdint.h>

#ifndef JELLINGSTONE_JS_NVS_H
#define JELLINGSTONE_JS_NVS_H
uint8_t js_nvs_ble_eddystone_my_org_id[10];
uint8_t js_nvs_ble_instance[6];
esp_err_t js_nvs_wlan_ssid(uint8_t ssid[32]);
esp_err_t js_nvs_wlan_psk(uint8_t ssid[64]);
esp_err_t js_nvs_init();

char* js_nvs_mqtt_url(); // Note: Returns NULL on NVS-Errors
char* js_nvs_mqtt_user(); // Note: Returns NULL on NVS-Errors
char* js_nvs_mqtt_password();  // Note: Returns NULL on NVS-Errors
char* js_nvs_mqtt_cert();  // Note: Returns NULL on NVS-Errors
#endif //JELLINGSTONE_JS_NVS_H
