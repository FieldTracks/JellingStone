/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org

    Based on esp-idf example code, distributed as public domain and CC0
*/

/****************************************************************************
*
* This file is for iBeacon APIs. It supports both iBeacon encode and decode.
*
* iBeacon is a trademark of Apple Inc. Before building devices which use iBeacon technology,
* visit https://developer.apple.com/ibeacon/ to obtain a license.
****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "esp_log.h"

#include "esp_gap_ble_api.h"
#include "esp_ibeacon_api.h"
#include "nvs.h"


const uint8_t uuid_zeros[ESP_UUID_LEN_128] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
esp_ble_ibeacon_head_t ibeacon_common_head;
esp_ble_ibeacon_vendor_t vendor_config;

/* For iBeacon packet format, please refer to Apple "Proximity Beacon Specification" doc */
/* Constant part of iBeacon data */
void ibeacon_init(){
    ibeacon_common_head = (esp_ble_ibeacon_head_t){
        .flags = {0x02, 0x01, 0x06},
        .length = 0x1A,
        .type = 0xFF,
        .company_id = 0x004C,
        .beacon_type = 0x1502
    };

    /* Vendor part of iBeacon data*/
    vendor_config = (esp_ble_ibeacon_vendor_t){
        .proximity_uuid = ESP_UUID,
        .major = endian_change_u16(get_ble_major()), //Major=ESP_MAJOR
        .minor = endian_change_u16(get_ble_minor()), //Minor=ESP_MINOR
        .measured_power = 0xC5
    };
}

int esp_ble_is_ibeacon_packet (uint8_t *adv_data, uint8_t adv_data_len){
    int result = 0;

    // Debugging things
    //ESP_LOGI("esp_ibeacon_api.c", "Data length: %02x  Header: %02x %02x %02x %02x %02x %02x %02x %02x %02x", adv_data_len, adv_data[0], adv_data[1], adv_data[2], adv_data[3], adv_data[4], adv_data[5], adv_data[6], adv_data[7], adv_data[8]);

    if ((adv_data != NULL) && (adv_data_len == 0x1E)){
        if (!memcmp(adv_data, (uint8_t*)&ibeacon_common_head, sizeof(ibeacon_common_head))){
            result = 1;
        }
    }

    return result;
}

esp_err_t esp_ble_config_ibeacon_data (esp_ble_ibeacon_vendor_t *vendor_config, esp_ble_ibeacon_t *ibeacon_adv_data){
    if ((vendor_config == NULL) || (ibeacon_adv_data == NULL) || (!memcmp(vendor_config->proximity_uuid, uuid_zeros, sizeof(uuid_zeros)))){
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&ibeacon_adv_data->ibeacon_head, &ibeacon_common_head, sizeof(esp_ble_ibeacon_head_t));
    memcpy(&ibeacon_adv_data->ibeacon_vendor, vendor_config, sizeof(esp_ble_ibeacon_vendor_t));

    return ESP_OK;
}

/* Major and Minor part are stored in big endian mode in iBeacon packet,
 * need to use this macro to transfer while creating or processing
 * iBeacon data */
uint16_t endian_change_u16(uint16_t value){
    return ((((value)&0xFF00)>>8) + (((value)&0xFF)<<8));
}
