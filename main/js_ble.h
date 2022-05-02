/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#ifndef JELLINGSTONE_JS_BLE_H
#define JELLINGSTONE_JS_BLE_H
void js_ble_scan_start(uint32_t durationInSeconds);
void js_ble_scan_stop();
void js_ble_scan_init();


typedef enum {
    JS_BEACON_NONE = 0,
    JS_BEACON_ALT_BEACON = 1,
    JS_BEACON_EDDISTONE_EID = 2,
    JS_BEACON_EDDISTONE_UID = 3
} js_ble_beacon_t;

/*
typedef struct {
    uint8_t organizational_unit[16];
    uint8_t remainder[4];
    int8_t calibrated_rssi;
    int8_t detected_rssi;
} js_altbeacon_t;

typedef struct {
    uint8_t namespace[10];
    uint8_t instance[6];
    int8_t calibrated_rssi;
    int8_t detected_rssi;
} jseddy_uid_t;

typedef struct {
    uint8_t eid[8];
    int8_t calibrated_rssi;
    int8_t detected_rssi;
} js_eddy_eid_t;
*/
#endif
