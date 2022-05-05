/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
#include <stdint.h>


#ifndef JELLINGSTONE_JS_DB_H
#define JELLINGSTONE_JS_DB_H
void js_db_process_altbeacon(uint8_t organizational_unit[16], uint8_t remainder[4], int8_t calibrated_rssi_1m, int8_t detected_rssi);
void js_db_process_eddy_eid(uint8_t eid[8], int8_t calibrated_rssi_0m, int8_t detected_rssi);
void js_db_process_eddy_uid(uint8_t namespace[10], uint8_t instance[6], int8_t calibrated_rssi_0m, int8_t detected_rssi);
#endif //JELLINGSTONE_JS_DB_H
