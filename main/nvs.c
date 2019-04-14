/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org

    Based on esp-idf example code, distributed as public domain and CC0

*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "nvs_flash.h"
#include "status.h"

#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"

static const char *TAG = "nvs.c";

static nvs_handle wifi;
static nvs_handle mqtt;
static nvs_handle ble;
static nvs_handle device;
static int init = false;

static char *wifi_ssid;
static char *wifi_pass;
static char *mqtt_uri;
static char *mqtt_user;
static char *mqtt_pass;
static char *mqtt_cert;
static uint16_t ble_major;
static uint16_t ble_minor;
static uint16_t ble_scan_interval;
static char *device_comment;

static int get_u16_from_nvs(nvs_handle handle, const char *attribute, uint16_t *out_value);
static char* get_string_from_nvs(nvs_handle handle, const char *attribute);
static void check_error(esp_err_t err, char *namespace);

void nvs_init(){
    esp_err_t err = nvs_open("wifi_config", NVS_READONLY, &wifi);
    check_error(err, "wifi");

    err = nvs_open("mqtt_config", NVS_READONLY, &mqtt);
    check_error(err, "mqtt");

    err = nvs_open("ble_config", NVS_READONLY, &ble);
    check_error(err, "ble");

    err = nvs_open("device_config", NVS_READONLY, &device);
    check_error(err, "device");

    init = true;
}

char* get_wifi_ssid(){
    if(wifi_ssid == NULL){
        wifi_ssid = get_string_from_nvs(wifi, "ssid");
    }
    return wifi_ssid;
}

char* get_wifi_pass(){
    if(wifi_pass == NULL){
        wifi_pass = get_string_from_nvs(wifi, "psk");
    }
    return wifi_pass;
}

char* get_mqtt_uri(){
    if(mqtt_uri == NULL){
        mqtt_uri = get_string_from_nvs(mqtt, "uri");
    }
    return mqtt_uri;
}

char* get_mqtt_user(){
    if(mqtt_user == NULL){
        mqtt_user = get_string_from_nvs(mqtt, "user");
    }
    return mqtt_user;
}

char* get_mqtt_pass(){
    if(mqtt_pass == NULL){
        mqtt_pass = get_string_from_nvs(mqtt, "pass");
    }
    return mqtt_pass;
}

char* get_mqtt_cert(){
    if(mqtt_cert == NULL){
        mqtt_cert = get_string_from_nvs(mqtt, "cert");
    }
    return mqtt_cert;
}

uint16_t get_ble_major(){
    if(ble_major == NULL){
        if(get_u16_from_nvs(ble, "beacon_major", &ble_major) != 0){
            ble_major = NULL;
        }
    }
    return ble_major;
}

uint16_t get_ble_minor(){
    if(ble_minor == NULL){
        if(get_u16_from_nvs(ble, "beacon_minor", &ble_minor) != 0){
            ble_minor = NULL;
        }
    }
    return ble_minor;
}

uint16_t get_ble_scan_interval(){
    if(ble_scan_interval == NULL){
        if(get_u16_from_nvs(ble, "scan_interval", &ble_scan_interval) != 0){
            ble_scan_interval = NULL;
        }
    }
    return ble_scan_interval;
}

char* get_device_comment(){
    if(device_comment == NULL){
        device_comment = get_string_from_nvs(device, "comment");
    }
    return device_comment;
}

static int get_u16_from_nvs(nvs_handle handle, const char *attribute, uint16_t *out_value){
    esp_err_t err;
    err = nvs_get_u16(handle, attribute, out_value);
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Error reading %s from NVS!", attribute);
        status_set(STATUS_NVS_MISSINGDATA);
        return -1;
    }
    return 0;
}

static char* get_string_from_nvs(nvs_handle handle, const char *attribute){
    size_t size = 0;
    esp_err_t err;
    err = nvs_get_str(handle, attribute, NULL, &size);
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Error reading %s from NVS!", attribute);
        status_set(STATUS_NVS_MISSINGDATA);
        return NULL;
    }

    char *location = (char *)malloc(size);
    err = nvs_get_str(handle, attribute, location, &size);
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Error reading %s from NVS!", attribute);
        status_set(STATUS_NVS_MISSINGDATA);
        return NULL;
    }

    return location;
}

static void check_error(esp_err_t err, char *namespace){
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening %s NVS handle!", namespace);
        status_set(STATUS_NVS_MISSINGDATA);
        return;
    }
}
