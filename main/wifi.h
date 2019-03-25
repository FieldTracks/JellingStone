/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

/**
  Connect to wifi, obain an IP-Adress
*/
#include "esp_wifi_types.h"
void start_wifi();
void status_wifi(wifi_ap_record_t *ap_info);
