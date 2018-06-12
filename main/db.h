/**
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
#include "esp_gap_ble_api.h"
void db_add(esp_bd_addr_t *bda, int rssi, int remoteRssi, uint16_t major, uint16_t minor, uint8_t *proximity_uuid);

char *db_dump_flush(char *timestmp);
