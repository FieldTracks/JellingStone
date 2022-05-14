#include <esp_log.h>
#include "js_nvs.h"
#include "js_status.h"
#include "js_fsm.h"
/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
static const char *TAG = "js_nvs";

static bool initialized = false;
static nvs_handle_t nvsHandle;

void js_nvs_init() {
    initialized = true;

    ESP_LOGI(TAG, "Loading configuration from NVS");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Unable to init nvs: %s", esp_err_to_name(err));
        js_status_set(nvs_missing);
        ESP_ERROR_CHECK(err);
    }

    if ((err = nvs_open("config", NVS_READONLY, &nvsHandle)) != ESP_OK) {
        ESP_LOGE(TAG, "Error opening nvs: %s", esp_err_to_name(err));
        js_status_set(nvs_missing);
        ESP_ERROR_CHECK(err);
    }

    // Eagerly cache own organisation for ble checks
    size_t cache_length = 10;
    ESP_ERROR_CHECK(nvs_get_blob(nvsHandle, "BLE_EDDY_ORG", js_nvs_ble_eddystone_my_org_id, &cache_length));
}

void js_nvs_read_str(const char *key, char *out_value, size_t *length) {
    esp_err_t err;
    if(!initialized) {
        js_nvs_init();
    }
    err = nvs_get_str(nvsHandle,key,out_value,length) ;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Unable to retrieve '%s' from NVS: %s", key, esp_err_to_name(err));
        js_status_set(nvs_missing);

        ESP_ERROR_CHECK(err);
    }
}

void js_nvs_read_i32(const char *key, int32_t *out_value, size_t *length) {
    esp_err_t err;
    if(!initialized) {
        js_nvs_init();
    }
    err = nvs_get_i32(nvsHandle,key,out_value) ;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Unable to retrieve '%s' from NVS: %s", key, esp_err_to_name(err));
        js_status_set(nvs_missing);

        ESP_ERROR_CHECK(err);
    }
}

void js_nvs_read_str_uint8_t(const char *key, uint8_t *out_value, size_t *length) {
    char *out_c = (char *) out_value;
    return js_nvs_read_str(key, out_c,length);
}

void js_nvs_wlan_ssid(uint8_t ssid[32]) {
    size_t length = 32;
    return js_nvs_read_str_uint8_t("WLAN_SSID",ssid,&length);
}
void js_nvs_wlan_psk(uint8_t psk[64]) {
    size_t length = 64;
    return js_nvs_read_str_uint8_t("WLAN_PSK",psk,&length);
}

static char* js_nvs_retr_string_2_calls(char *key) {
    size_t length;
    js_nvs_read_str(key,NULL,&length);
    char *ret_val = malloc(length);
    js_nvs_read_str(key, ret_val, &length);
    return ret_val;
}
char* js_nvs_mqtt_url()  {
    return js_nvs_retr_string_2_calls("MQTT_URL");
}
char* js_nvs_mqtt_user() {
    return js_nvs_retr_string_2_calls(("MQTT_USER"));
}
char* js_nvs_mqtt_password() {
    return js_nvs_retr_string_2_calls(("MQTT_PWD"));
}
char* js_nvs_mqtt_cert() {
    return js_nvs_retr_string_2_calls(("MQTT_CERT"));
}



