/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
void mqtt_start();
void mqtt_restart();
void mqtt_init();
void mqtt_publish(uint8_t mac_id[6], char* message);
void mqtt_publish_msg(char *channel, char* message);
