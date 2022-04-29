#include "nvs_flash.h"
#include <stdint.h>

#ifndef JELLINGSTONE_JS_NVS_H
#define JELLINGSTONE_JS_NVS_H

//void js_nvs_read_str(const char *key, char *out_value, size_t *length);
//void js_nvs_read_str_uint8_t(const char *key, uint8_t *out_value, size_t *length);

//void js_nvs_read_i32(const char *key, int32_t *out_value, size_t *length);

void js_nvs_wlan_ssid(uint8_t ssid[32]);
void js_nvs_wlan_psk(uint8_t ssid[64]);

#endif //JELLINGSTONE_JS_NVS_H
