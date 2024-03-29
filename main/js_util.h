/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#ifndef JS_ERROR_CHECK
#define JS_ERROR_CHECK(x) do {                                         \
        esp_err_t err_rc_ = (x);                                        \
        if (unlikely(err_rc_ != ESP_OK)) {                              \
                                    return err_rc_;                 \
        }                                                               \
    } while(0)
#endif


#ifndef JELLINGSTONE_UTIL_H
#define JELLINGSTONE_UTIL_H


#include <stdint.h>

void js_6bytestr(uint8_t p[6], char dest[18]);
void js_uuid2str(uint8_t *uuid, char dest[48]);
void js_mac2strBLE(const uint8_t p[6], char dest[18]);
void js_id10bytes2str(uint8_t *p, char dest[30]);
void js_mymac_str(char dest[18]);

#endif //JELLINGSTONE_UTIL_H
