# JellingStone
This repository is part of the [Fieldtracks](https://fieldtracks.org/) project, which aims at creating a tracking system to be used in field exercises by relief organizations.

## Building

Please make yourself comfortable with ESP-IDF. JellingStone based on ESP-IDF a more-or-less standard way.
It is built like other ESP-IDF Applications:

```
git clone https://github.com/FieldTracks/JellingStone.git
```

Finally run `make flash` to compile the project and flash it to an attached ESP32. Debug output can be watched with `make monitor` (or run them together with `make flash monitor`).

### Flashing wifi and mqtt credentials to the ESP32
This will not run automatically during `make flash`.

1) Copy `nvs_data.csv.example` to `nvs_data.csv`
2) Edit the credentials
3) Run `./flash_nvs`

## Running JellingStone

The current design is designed to run on ESP32-WROOM dev-boards having a blue and a read LED. LED status indication:
 
* red (solely): Problems starting the APP. Jelling-Stone is not Active
* red + blue blinking
  * 1 Hz: Booting, no WLAN connection
  * 5 Hz: IP connection, no MQTT connection
  * 1 Hz / 5 Hz oscilating: Unrecoverable error occured. Check monitor log
  * Short "twinkle": Report is submitted over MQTT
* red + blue: Device is operating (i.e. WLAN + MQTT connection, BLE is scanning and beaconing)

## MQTT messages

### SCAN report
Topic: `JellingStone/<MAC of BLE>/scan`

#### Report structure and header

| Byte     | Description                                                                                                                                                                                        |
|----------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0        | Version=0x01, Reserved: 0x7B for JSON Payloads                                                                                                                                                     |
| 1-4      | Report-ID / 32-Bit timestamp (seconds since Epoch), Big-Endian / Network Byte Order                                                                                                                |
| 5        | Message-Sequence Number ID (signed), unique per report, starts at 0x01 in each report, incremented per message, negative number indicates final message, e.g. 0xFF=-1, if there's just one message |
| 6        | unsigned, number of beacon data segments in this message (report has more, if and only if there are more messages)                                                                                 |
| 7 - 1119 | 0...255 (up to 255) segments of beacon data                                                                                                                                                        |

#### Beacon Data segment

| Byte   | Description                                                                                                                                                                                                                                                                                                                                                 |
|--------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0      | Type and length of Beacon ID in Bytes. <br /> `0x14` AltBeacon <br /> `0x08` Eddystone EID <br /> `0x10` Eddystone UID <br /> `0x06` Eddystone UID, configured organization <br > `0x01` Eddystone UID, configured organization, Instance-ID <= 255 (i.e. 1 Byte) <br /> `0x02` Eddystone UID, configured organization, Instance-ID <= 65536 (i.e. 2 Bytes) |
| 1      | signed, Detected RSSI in dBm + 100 (i.e. -228 dBm to 27 dBm) encoded as -128 to 127                                                                                                                                                                                                                                                                         |
| 2 - 21 | Beacon ID, variable length                                                                                                                                                                                                                                                                                                                                  |


Beacon data is encoded on a Type-Value basis, whereas the byte value of the type corresponds to the length of the ID-Value


### JSON status report

Topic: `JellingStone/<MAC of BLE>/status`

See content for details.

## License
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    Military usage is forbidden.

    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org

