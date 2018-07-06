/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org

    Based on esp-idf example code, distributed as public domain and CC0

*/

#include "esp_gap_ble_api.h"
void mac2str(uint8_t p[6], char dest[18]){
  sprintf(dest, "%02x:%02x:%02x:%02x:%02x:%02x",
          p[0], p[1], p[2], p[3], p[4], p[5]);
}

void uuid2str(uint8_t *p, char dest[48]){
  sprintf(dest, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
          p[0], p[1], p[2], p[3], p[4], p[5],
          p[6], p[7], p[8], p[9], p[10], p[11],
          p[12], p[13], p[14], p[15]);
}
