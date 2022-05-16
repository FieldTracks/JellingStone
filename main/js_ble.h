/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#ifndef JELLINGSTONE_JS_BLE_H
#define JELLINGSTONE_JS_BLE_H
#include "esp_err.h"

esp_err_t js_ble_scan_start();
void js_ble_scan_stop();
esp_err_t js_ble_scan_init();

typedef enum {
    JS_BEACON_NONE = 0, // No beacon, no data
    JS_BEACON_ALT_BEACON = 20, // Beacon data is 20 Bytes
    JS_BEACON_EDDYSTONE_EID = 8, // Beacon data is 8 Bytes
    JS_BEACON_EDDYSTONE_UID = 16, // Beacon data is 16-Bytes (10-Bytes org, 6 bytes instance)
    JS_BEACON_EDDYSTONE_UID_MY_NETWORK = 6 , // organizational_unit configured in NVS - hence 6 bytes of data
    JS_BEACON_EDDYSTONE_UID_MY_NETWORK_ONE_BYTE = 1 , // organizational_unit configured in NVS - Instance ID is just one byte long
    JS_BEACON_EDDYSTONE_UID_MY_NETWORK_TWO_BYTES = 2 , // organizational_unit configured in NVS - Instance ID is just 2 bytes long

} js_ble_beacon_t;
size_t payload_length_of_type(js_ble_beacon_t type);

esp_err_t js_ble_start_beacon();
esp_err_t js_ble_stop_beacon();

#endif
