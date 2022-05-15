/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#ifndef JELLINGSTONE_JS_MQTT_H
#define JELLINGSTONE_JS_MQTT_H

#include <stdint.h>
#include "js_ble.h"

esp_err_t js_mqtt_start();
esp_err_t js_mqtt_stop();
esp_err_t js_mqtt_restart();
esp_err_t js_mqtt_init();
esp_err_t js_mqtt_publish_report(uint8_t *message, int len, int *msg_id_out);
esp_err_t js_mqtt_publish_status(char *message,int *msg_id_out);

#endif //JELLINGSTONE_JS_MQTT_H
