This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    Military usage is forbidden.

    You should have received a copy of COLi along with JellingStone.
    If not, please contact info@fieldtracks.org

Note: espmqtt must be installed to build this project c.f. https://github.com/tuanpmt/espmqtt

Flashing wifi and mqtt credentials to the ESP32
================================================

This will not run during "make flash"

1) Copy "nvs_data.csv.example" to "nvs_data.csv"
2) Edit the credentials
3) Run "make flash_nvs"

IMPORTANT: If the offset of the nvs partition gets updated the offset in the Makfile needs to be updated as well