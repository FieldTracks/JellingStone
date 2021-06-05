/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
#include "esp_gap_ble_api.h"
void mac2str(uint8_t p[6], char dest[18]);
void uuid2str(uint8_t *uuid, char dest[48]);
void mac2strBLE(const uint8_t p[6], char dest[18]);
void id10bytes2str(uint8_t *p, char dest[30]);
