#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <esp_bt_device.h>

/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
    Based on esp-idf example code, distributed as public domain and CC0
*/

void js_6bytestr(uint8_t p[6], char dest[18]){
    sprintf(dest, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
}

void js_uuid2str(uint8_t *p, char dest[48]){
    sprintf(dest, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5],
            p[6], p[7], p[8], p[9], p[10], p[11],
            p[12], p[13], p[14], p[15]);
}

void js_mac2strBLE(const uint8_t p[6], char dest[18]){
    sprintf(dest, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
}

void js_id10bytes2str(uint8_t *p, char dest[30]){
    sprintf(dest, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5],
            p[6], p[7], p[8], p[9]);
}

void js_mymac_str(char dest[18]) {
    uint8_t mac[6];
    memcpy(mac,esp_bt_dev_get_address(),6);
    js_6bytestr(mac, dest);
}
