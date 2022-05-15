/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#include <esp_log.h>
#include <esp_chip_info.h>
#include <esp_app_format.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include "js_status_report.h"
#include "cJSON.h"
#include "js_nvs.h"
#include "js_util.h"
#include "js_ntp.h"
#include "js_mqtt.h"

static const char *TAG = "js_status_report";
static uint8_t ssid[33] = {'\0'};
static char eddy_org_str[30] =  {'\0'};
static char eddy_instance_str[18] = {'\0'};
static char timestmp[128] = {'\0'};

esp_err_t js_status_report_send() {

    ESP_LOGI(TAG, "Generating status report");
    cJSON *root, *app_info, *mem_info, *nvs;
    root = cJSON_CreateObject();
    app_info = cJSON_AddObjectToObject(root,"app_info");
    mem_info = cJSON_AddObjectToObject(root,"mem_info");
    nvs = cJSON_AddObjectToObject(root,"nvs");
    const esp_app_desc_t *app_desc = esp_ota_get_app_description();

    cJSON_AddStringToObject(app_info, "version", app_desc->version);
    cJSON_AddStringToObject(app_info, "project_name", app_desc->project_name);
    cJSON_AddStringToObject(app_info, "time", app_desc->time);
    cJSON_AddStringToObject(app_info, "date", app_desc->date);
    cJSON_AddStringToObject(app_info, "idf_ver", app_desc->idf_ver);

    cJSON_AddNumberToObject(mem_info,"free_heap_size", esp_get_free_heap_size());
    cJSON_AddNumberToObject(mem_info,"min_heap_size", esp_get_minimum_free_heap_size());

    js_id10bytes2str(js_nvs_ble_eddystone_my_org_id,eddy_org_str);
    js_nvs_wlan_ssid(ssid);
    js_6bytestr(js_nvs_ble_instance, eddy_instance_str);
    cJSON_AddStringToObject(nvs, "WLAN_SSID", (char *) ssid);
    cJSON_AddStringToObject(nvs, "MQTT_URL", js_nvs_mqtt_url());
    cJSON_AddStringToObject(nvs, "MQTT_USER", js_nvs_mqtt_user());
    cJSON_AddStringToObject(nvs, "BLE_EDDY_ORG", eddy_org_str);
    cJSON_AddStringToObject(nvs, "BLE_EDDY_INST", eddy_instance_str);

    js_ntp_time_str(timestmp);
    cJSON_AddStringToObject(root, "date_time", timestmp);

    char *result = cJSON_PrintUnformatted(root);
    int msg_id;
    esp_err_t  ret = js_mqtt_publish_status(result,&msg_id);
    free(result);
    cJSON_Delete(root);
    return ret;
}