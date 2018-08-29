/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
void nvs_init();

char* get_wifi_ssid();
char* get_wifi_pass();
char* get_mqtt_uri();
char* get_mqtt_user();
char* get_mqtt_pass();
char* get_mqtt_cert();
uint16_t get_ble_major();
uint16_t get_ble_minor();
uint16_t get_ble_scan_interval();
char* get_device_comment();
