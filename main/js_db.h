/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
#include <stdint.h>
#include "js_ble.h"

#ifndef JELLINGSTONE_JS_DB_H
#define JELLINGSTONE_JS_DB_H
// ~32 Byte per Beacon, not packed
typedef struct db_entry {
    js_ble_beacon_t type;
    int8_t max_rssi;
    uint8_t data[20]; // At most - entries can be smaller
} db_entry_t;

// Needs ~ 8 KiB on heap
db_entry_t js_db_database[256];

void js_db_store_beacon(uint8_t *data, int8_t detected_rssi, js_ble_beacon_t type);
int js_db_submit_over_mqtt();
void js_db_clear();
#endif //JELLINGSTONE_JS_DB_H
