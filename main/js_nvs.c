#include <esp_log.h>
#include "js_nvs.h"
//
// Created by jan on 24/04/2022.
//
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
        ESP_ERROR_CHECK(err);
    }


    if ((err = nvs_open("config", NVS_READONLY, &nvsHandle)) != ESP_OK) {
        ESP_LOGE(TAG, "Error opening nvs: %s", esp_err_to_name(err));
        ESP_ERROR_CHECK(err);
    }
}

void js_nvs_read_str(const char *key, char *out_value, size_t *length) {
    esp_err_t err;
    if(!initialized) {
        js_nvs_init();
    }
    err = nvs_get_str(nvsHandle,key,out_value,length) ;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Unable to retrieve '%s' from NVS: %s", key, esp_err_to_name(err));
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