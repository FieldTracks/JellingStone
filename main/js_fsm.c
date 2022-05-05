/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

// Jelling-Stone Finite State
#include <esp_log.h>
#include "js_fsm.h"
#include "js_wlan.h"
#include "js_ntp.h"
#include "js_mqtt.h"
#include "js_ble.h"

static char* TAG = "js_fsm";

static enum state_t {
    ip_disconnected = 0,
    ip_connected = 1,
    mqtt_connected = 2,
    scanning = 3,
    publishing_scan = 4,
    publishing_status = 5
} global_state;

void js_on_mqtt_connected() {
    ESP_LOGD(TAG, "on MQTT connected");
    global_state = mqtt_connected;
    js_ble_scan_start(0);
}
void js_on_mqtt_disconnected() {
    if(global_state >= mqtt_connected) {
        global_state = ip_connected;
    }
    js_mqtt_start();
}
void js_on_mqtt_published() {

}
void js_on_mqtt_error() {
    if(global_state >= mqtt_connected) {
        global_state = ip_connected;
        js_mqtt_restart();
    }
}

void js_fsm_app_start() {
    ESP_LOGI(TAG, "FSM: Starting - status is ip_disconnected.");
    global_state = ip_disconnected;
    js_wlan_connect();
    js_mqtt_init();
    js_ble_scan_init();
}

void js_on_ip_recv() {
    ESP_LOGI(TAG, "IP received");
    if(global_state <= ip_disconnected) {
        global_state = ip_connected;
        js_ntp_obtain_time();
        js_mqtt_start();
    } else {
        ESP_LOGI(TAG, "Unknown transition: js_on_ip_recv in state %d", global_state);
    }
}

void js_on_wlan_disconnected() {
    ESP_LOGI(TAG, "WLAN disconnect in state %d", global_state);
    global_state = ip_disconnected;
    js_wlan_connect();
    js_mqtt_stop();
}
void js_on_ble_finished(){
    ESP_LOGI(TAG, "BLE Scan completed");
    global_state = publishing_scan;
}