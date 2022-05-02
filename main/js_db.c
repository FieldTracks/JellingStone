/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#include <esp_log.h>
#include "js_db.h"

static char *TAG="js_db";
void js_db_process_altbeacon(uint8_t organizational_unit[16], uint8_t remainder[4], int8_t calibrated_rssi, int8_t detected_rssi) {
    ESP_LOGD(TAG,"Processing js_db_process_altbeacon");
}
void js_db_process_eddy_eid(uint8_t eid[8], int8_t calibrated_rssi, int8_t detected_rssi) {
    ESP_LOGD(TAG,"Processing js_db_process_eddy_eid");
}
void js_db_process_eddy_uid(uint8_t namespace[10], uint8_t instance[6], int8_t calibrated_rssi, int8_t detected_rssi) {
    ESP_LOGD(TAG,"Processing js_db_process_eddy_uid");
}
