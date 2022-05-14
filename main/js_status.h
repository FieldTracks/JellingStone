/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#ifndef JELLINGSTONE_JS_STATUS_H
#define JELLINGSTONE_JS_STATUS_H

#include "js_fsm.h"
void js_status_booting();

void js_status_set(int status);

void js_status_ack_sent();


#endif //JELLINGSTONE_JS_STATUS_H
