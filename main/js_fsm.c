#include <sys/cdefs.h>
/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

// Jelling-Stone Finite State
#include <esp_log.h>
#include <driver/gpio.h>
#include "js_fsm.h"
#include "js_wlan.h"
#include "js_ntp.h"
#include "js_mqtt.h"
#include "js_ble.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "js_db.h"
#include "js_nvs.h"

static char* TAG = "js_fsm";
static TaskHandle_t main_task;
static TaskHandle_t blinking_task;

static void js_fsm_blink(int count, int freq); // Keep blinking-stuff internal
static void js_blink_task_worker(void *pvParameter); // Keep blinking-stuff internal

#define SCAN_PERIOD_SECONDS 8
#define BLINK_GPIO 2 // Blue LED on devboard

typedef enum {
    error = -1,
    booting = 1,
    ip_disconnected = 2,
    ip_connected = 3,
    scan_and_publish = 4,
} js_state_t;

js_state_t global_status;

static void set_state_internal(js_state_t new_status) {
    if(new_status == booting) { // Start blinking-task when booting
        xTaskCreate(&js_blink_task_worker, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, &blinking_task);
    }

    if(global_status > error) { // Error once => Error all the time
        global_status = new_status;
    }
    vTaskResume(blinking_task); // Resume blinking if halted.

}

static void check_error(esp_err_t status) { // When running into an error: suspend main task, let thins come to a rest.
    if(status != ESP_OK) {
        set_state_internal(error);
        vTaskSuspend(main_task);
    }
}

void js_on_mqtt_connected() {
    ESP_LOGD(TAG, "on MQTT connected");
    set_state_internal(scan_and_publish);
    xTaskNotify(main_task, 0, eNoAction); // If MQTT is connected: Restart waiting task
}

void js_on_mqtt_disconnected() {
    if(global_status == scan_and_publish) {
        set_state_internal(ip_connected);
    }
    js_mqtt_start();
}

void js_on_mqtt_error() {
    if(global_status == scan_and_publish) {
        set_state_internal(ip_connected);
        js_mqtt_restart();
    }
}

_Noreturn void js_fsm_app_start() {
    main_task = xTaskGetCurrentTaskHandle();
    set_state_internal(booting);
    ESP_LOGI(TAG, "FSM: Starting - status is ip_disconnected.");
    check_error(js_nvs_init());
    check_error(js_wlan_connect());
    check_error(js_mqtt_init());
    check_error(js_ble_scan_init());

    // Main Loop
    uint32_t ulNotifiedValue;
    while(true) {
        ESP_LOGI(TAG, "Entering main loop - global state s %d", global_status);
        bool scan_started = false;
        if(global_status == scan_and_publish) {
            ESP_LOGI(TAG,"Start scanning");
            check_error(js_ble_scan_start());
            scan_started = true;
        } else {
            ESP_LOGI(TAG, "Not starting scan - wrong state %d", global_status);
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
            if(global_status == scan_and_publish) { // Makes no sense to submit something if we're not connected
                ESP_LOGI(TAG, "Mqtt: Go: Global state is %d", global_status);
                if(js_db_submit_over_mqtt() > 0) { // Sucessful submit
                    vTaskResume(blinking_task); // Short blink, "flash"
                }
                vTaskResume(&blinking_task);
            } else { // Empty database if not connected
                ESP_LOGI(TAG, "Mqtt: Oops: Global state is %d - clearing db", global_status);
                js_db_clear();
            }
        } else {
            ESP_LOGI(TAG, "Resuming - not submitting data. Global state is: %d", global_status);

        }
        vTaskDelay(100 / portTICK_PERIOD_MS); // Give the system 100ms to process MQTT, TCP, etc.
    }

}

void js_on_ip_recv() {
    ESP_LOGI(TAG, "IP received");
    if(global_status <= ip_disconnected) {
        set_state_internal(ip_connected);
        js_ntp_obtain_time();
        check_error(js_mqtt_start());
    } else {
        ESP_LOGI(TAG, "Unknown transition: js_on_ip_recv in state %d", global_status);
    }
}

void js_on_wlan_disconnected() {
    js_mqtt_stop(); // Ignore error - the connection might already be dead
    ESP_LOGI(TAG, "WLAN disconnect in state %d", global_status);
    set_state_internal(ip_disconnected);
    check_error(js_wlan_connect());
}

void js_on_db_full() {
    xTaskNotify(main_task, 0, eNoAction);
}


void js_fsm_blink(int count, int freq) {
    gpio_set_level(BLINK_GPIO, 1);

    for(int i = 0; i < count; i++) {
        vTaskDelay(freq / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(freq / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO, 1);
    }
}
_Noreturn void js_blink_task_worker(void *pvParameter)
{
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    while(1) {
        gpio_set_level(BLINK_GPIO, 1);
        switch (global_status) {
            case error:
                js_fsm_blink(1, 100);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                js_fsm_blink(1, 500);
                break;
            case booting: // NOLINT(bugprone-branch-clone)
                js_fsm_blink(1, 1000);
                break;
            case ip_disconnected: // NOLINT(bugprone-branch-clone)
                js_fsm_blink(1, 1000);
                break;
            case ip_connected:
                js_fsm_blink(1, 100);
                break;
            case scan_and_publish:
                js_fsm_blink(1, 100);
                vTaskSuspend( NULL );
                break;
            default:
                ESP_LOGW(TAG, "Hey, I don't know how to blink for status %d",global_status);
                vTaskSuspend( NULL );
                break;
        }
    }
}