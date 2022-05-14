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
void js_nvs_wlan_ssid(uint8_t ssid[32]);
void js_nvs_wlan_psk(uint8_t ssid[64]);
char* js_nvs_mqtt_url();
char* js_nvs_mqtt_user();
char* js_nvs_mqtt_password();
char* js_nvs_mqtt_cert();
#endif //JELLINGSTONE_JS_NVS_H
