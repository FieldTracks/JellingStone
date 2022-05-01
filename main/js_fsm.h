/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#ifndef JELLINGSTONE_JS_FSM_H
#define JELLINGSTONE_JS_FSM_H
void js_fsm_app_start();
void js_on_ip_recv();
void js_on_wlan_disconnected();
void js_on_mqtt_connected();
void js_on_mqtt_disconnected();
void js_on_mqtt_published();
void js_on_mqtt_error();
#endif //JELLINGSTONE_JS_FSM_H
