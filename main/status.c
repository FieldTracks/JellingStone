/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "status.h"
#include "limits.h"
#include "freertos/semphr.h"
/* Can run 'make menuconfig' to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 2 // Blue LED on devboard

static int wifiStatus = STATUS_BOOTING;
static int mqttStatus = STATUS_BOOTING;

const char *MY_TAG = "status.c";

TaskHandle_t runningTask;

void blink(int count, int freq) {
  gpio_set_level(BLINK_GPIO, 1);

  for(int i = 0; i < count; i++) {
    vTaskDelay(freq / portTICK_PERIOD_MS);
    gpio_set_level(BLINK_GPIO, 0);
    vTaskDelay(freq / portTICK_PERIOD_MS);
    gpio_set_level(BLINK_GPIO, 1);
  }

}

void blink_task(void *pvParameter)
{
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    while(1) {
      gpio_set_level(BLINK_GPIO, 1);
      if(mqttStatus == STATUS_MQTT_CONNECTED) {
        blink(1,80);
        vTaskSuspend( NULL );
      } else if ( wifiStatus == STATUS_WIFI_CONNECTED){
        blink(1,100);
      } else {
        blink(1,1000);
      }
    }
}


void status_ack_sent(){
  vTaskResume( runningTask );
}

void status_booting(){
  xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, &runningTask);
}
void resetBlinking() {
  vTaskSuspend(runningTask);
  vTaskDelete(runningTask);
  xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, &runningTask);
}


void status_set(int status){
  ESP_LOGI(MY_TAG, "Got status %d",status);
  if (status == STATUS_WIFI_CONNECTED) {
      wifiStatus = STATUS_WIFI_CONNECTED;
  } else if (status == STATUS_MQTT_CONNECTED ) {
    mqttStatus = STATUS_MQTT_CONNECTED;
  } else if (status == STATUS_WIFI_DISCONNECTED) {
    wifiStatus = STATUS_BOOTING;
  } else if (status == STATUS_MQTT_DISCONNECTED) {
    mqttStatus = STATUS_BOOTING;
  }
  resetBlinking();
}
