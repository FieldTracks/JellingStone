#include <sys/cdefs.h>
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "js_db.h"
#include "js_status.h"

static char* TAG = "js_fsm";
static TaskHandle_t main_task;

#define SCAN_PERIOD_SECONDS 8


js_state_t global_state;

void js_on_mqtt_connected() {
    ESP_LOGD(TAG, "on MQTT connected");
    global_state = scan_and_publish;
    js_status_set(scan_and_publish);
    xTaskNotify(main_task, 0, eNoAction); // If MQTT is connected: Restart waiting task
}

void js_on_mqtt_disconnected() {
    if(global_state == scan_and_publish) {
        global_state = ip_connected;
        js_status_set(ip_connected);
    }
    js_mqtt_start();
}

void js_on_mqtt_error() {
    if(global_state == scan_and_publish) {
        global_state = ip_connected;
        js_status_set(ip_connected);
        js_mqtt_restart();
    }
}

_Noreturn void js_fsm_app_start() {
    ESP_LOGI(TAG, "FSM: Starting - status is ip_disconnected.");
    js_wlan_connect();
    js_mqtt_init();
    js_ble_scan_init();
    main_task = xTaskGetCurrentTaskHandle();

    // Main Loop
    uint32_t ulNotifiedValue;
    while(true) {
        ESP_LOGI(TAG, "Entering main loop - global state s %d", global_state);
        bool scan_started = false;
        if(global_state == scan_and_publish) {
            ESP_LOGI(TAG,"Start scanning");
            js_ble_scan_start();
            scan_started = true;
        } else {
            ESP_LOGI(TAG, "Not starting scan - wrong state %d", global_state);
        }
        ESP_LOGI(TAG, "Sleeping for 4 secs - heap free: %d", esp_get_free_heap_size());

        xTaskNotifyWait( 0x00,      /* Clear no bits on entry */
                         ULONG_MAX,      /* Clear all bits on exit */
                         &ulNotifiedValue,
                         SCAN_PERIOD_SECONDS * 1000 / portTICK_PERIOD_MS // Scan period: 4 seconds
                         );
        if(scan_started == true) {
            ESP_LOGI(TAG, "Stopping Scan, submitting data");
            js_ble_scan_stop();
            if(global_state == scan_and_publish) { // Makes no sense to submit something if we're not connected
                ESP_LOGI(TAG, "Mqtt: Go: Global state is %d", global_state);
                js_db_submit_over_mqtt();
            } else { // Empty database if not connected
                ESP_LOGI(TAG, "Mqtt: Oops: Global state is %d - clearing db", global_state);
                js_db_clear();
            }
        } else {
            ESP_LOGI(TAG, "Resuming - not submitting data. Global state is: %d", global_state);

        }
        vTaskDelay(100 / portTICK_PERIOD_MS); // Give the system 100ms to process MQTT, TCP, etc.
    }

}

void js_on_ip_recv() {
    ESP_LOGI(TAG, "IP received");
    if(global_state <= ip_disconnected) {
        global_state = ip_connected;
        js_status_set(ip_connected);
        js_ntp_obtain_time();
        js_mqtt_start();
    } else {
        ESP_LOGI(TAG, "Unknown transition: js_on_ip_recv in state %d", global_state);
    }
}

void js_on_wlan_disconnected() {
    ESP_LOGI(TAG, "WLAN disconnect in state %d", global_state);
    global_state = ip_connected;
    js_status_set(ip_connected);

    js_wlan_connect();
    js_mqtt_stop();
}

void js_on_db_full() {
    xTaskNotify(main_task, 0, eNoAction);

}