/*
This file is part of JellingStone - (C) The Fieldtracks Project
        JellingStone is distributed under the civilian open source license (COSLi).
You should have received a copy of COSLi along with JellingStone.
If not, please contact info@fieldtracks.org
        Based on esp-idf example code, distributed as public domain and CC0
*/

#ifndef JELLINGSTONE_JS_NTP_H
#define JELLINGSTONE_JS_NTP_H

void js_ntp_obtain_time();

void js_ntp_time_str(char dest[128]);
#endif //JELLINGSTONE_JS_NTP_H
