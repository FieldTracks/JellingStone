/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#ifndef JELLINGSTONE_JS_MQTT_H
#define JELLINGSTONE_JS_MQTT_H

#include <stdint.h>

void js_mqtt_start();
void js_mqtt_stop();
void js_mqtt_restart();
void js_mqtt_init();
void js_mqtt_publish(uint8_t mac_id[6], char* message);
void js_mqtt_publish_msg(char *channel, char* message);

#endif //JELLINGSTONE_JS_MQTT_H
