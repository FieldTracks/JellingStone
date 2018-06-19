/**
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#define DB_UUID_LENGTH_IN_BYTE 16
#include "esp_gap_ble_api.h"
void db_add(int rssi, int remoteRssi, uint16_t major, uint16_t minor, uint8_t *proximity_uuid);

char *db_dump_flush(char *timestmp);
