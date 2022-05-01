/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
#include <string.h>

#include "esp_log.h"
#include "js_nvs.h"
#include "js_wlan.h"
#include "js_fsm.h"
static const char *TAG = "app_main";

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    js_fsm_app_start();
}
