/**
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#define DB_NETWORK_ID_LENGTH_IN_BYTE 10
#define DB_BEACON_ID_LENGTH_IN_BYTE 6
#include "esp_gap_ble_api.h"
#include "esp_wifi_types.h"

void db_add_mac(uint8_t *mac_address, int rssi);
void db_add_eddystone_uid(uint8_t *mac_address, int rssi, uint8_t *network_id,  uint8_t *beacon_id);

char *db_dump_flush(char *timestmp, uint8_t *own_ble_mac_address);
