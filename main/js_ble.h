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


//typedef struct {
//    uint8_t organizational_unit[16];
//    uint8_t remainder[4];
//    int8_t calibrated_rssi;
//} js_altbeacon_t;
//
//typedef struct {
//    uint8_t namespace[10];
//    uint8_t instance[6];
//    int8_t calibrated_rssi;
//} jseddy_uid_t;
//
//typedef struct {
//    uint8_t eid[8];
//    int8_t calibrated_rssi;
//} js_eddy_eid_t;

#endif
