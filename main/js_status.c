#include <sys/cdefs.h>
/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "js_status.h"

#define BLINK_GPIO 2 // Blue LED on devboard

// TODO TODO TODO TODO
// This is basically copy & paste from the original implementation
// This is really nasty code
// Refactor this and move it into js_fsm.c

static int wifiStatus = booting;
static int mqttStatus = booting;

static const char *MY_TAG = "status.c";

static TaskHandle_t runningTask;

void blink(int count, int freq) {
    gpio_set_level(BLINK_GPIO, 1);

    for(int i = 0; i < count; i++) {
        vTaskDelay(freq / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(freq / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO, 1);
    }

}

_Noreturn void blink_task(void *pvParameter)
{
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    while(1) {
        gpio_set_level(BLINK_GPIO, 1);
        if(mqttStatus == scan_and_publish) {
            blink(1,80);
            vTaskSuspend( NULL );
        } else if ( wifiStatus == ip_connected){
            blink(1,100);
        } else if ( wifiStatus == nvs_missing || mqttStatus == nvs_missing){
            blink(1, 75);
            blink(1, 200);
        } else {
            blink(1,1000);
        }
    }
}


void js_status_ack_sent(){
    vTaskResume( runningTask );
}

void js_status_booting(){
    xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, &runningTask);
}
void js_resetBlinking() {
    vTaskSuspend(runningTask);
    vTaskDelete(runningTask);
    xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, &runningTask);
}


void js_status_set(js_state_t status){
    ESP_LOGI(MY_TAG, "Got status %d",status);
    switch (status) {
        case nvs_missing:
            wifiStatus = nvs_missing;
            mqttStatus = nvs_missing;
            break;
        case booting: // NOLINT(bugprone-branch-clone)
            wifiStatus = booting;
            mqttStatus = booting;
            break;
        case ip_disconnected: // NOLINT(bugprone-branch-clone)
            wifiStatus = booting;
            mqttStatus = booting;
            break;
        case ip_connected:
            wifiStatus = ip_connected;
            mqttStatus = booting;
            break;
        case scan_and_publish:
            wifiStatus = ip_connected;
            mqttStatus = scan_and_publish;
            break;
        default:
            ESP_LOGE(MY_TAG, "No blinking defined for status %d", status);
            break;
    }
    js_resetBlinking();
}