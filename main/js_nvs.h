#include "nvs_flash.h"

#ifndef JELLINGSTONE_JS_NVS_H
#define JELLINGSTONE_JS_NVS_H
void js_nvs_init();
u_int8_t *js_nvs_wlan_ssid();
u_int8_t *js_nvs_wlan_psk();
char *js_nvs_mqtt_url();
char *js_nvs_mqtt_user();
char *js_nvs_mqtt_pwd();
char *js_nvs_beacon_id();
uint32_t nvs_beacon_interval_ms();
uint32_t nvs_scan_interval_ms();

#endif //JELLINGSTONE_JS_NVS_H
