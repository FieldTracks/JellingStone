/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/apps/sntp.h"

static const char *TAG = "ntp.c";

static void initialize_sntp(void);

void time_str(char dest[128]){
  time_t now = 0;
  struct tm timeinfo = { 0 };

  // wait for time to be set
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(dest, 127, "%FT%TZ", &timeinfo);
   //ESP_LOGI(TAG, "The current date/time UTC is: %s", dest);

}

void obtain_time()
{
    initialize_sntp();
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}
