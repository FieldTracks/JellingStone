/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org

*/
#include <string.h>
#include "esp_log.h"
#include "db.h"
#include "cJSON.h"
#include "util.h"
#include "nvs.h"
#include "esp_timer.h"

#include "esp_ibeacon_api.h"

static const char *TAG = "db.c";

typedef struct _db_entry {
    uint8_t mac_address[6];
    uint8_t proximity_uuid[DB_UUID_LENGTH_IN_BYTE];
    int min_rssi;
    int max_rssi;
    int total_rssi;
    int count;
    int remoteRssi;
    uint16_t major;
    uint16_t minor;
    uint8_t isBeacon;
} db_entry;

#define DB_SIZE 400

static db_entry database[DB_SIZE];
static int cnt = 0;

void db_add(uint8_t *mac_address, int rssi, int remoteRssi, uint16_t major, uint16_t minor, uint8_t *proximity_uuid, uint8_t isBeacon){
  for(int i = 0; i < cnt;i++){
    if(memcmp(mac_address, database[i].mac_address, 6) == 0 && memcmp(proximity_uuid,database[i].proximity_uuid,DB_UUID_LENGTH_IN_BYTE) == 0 && major == database[i].major && minor == database[i].minor ){
      database[i].min_rssi = (database[i].min_rssi < rssi) ? database[i].min_rssi : rssi;
      database[i].max_rssi = (database[i].max_rssi > rssi) ? database[i].max_rssi : rssi;
      database[i].total_rssi += rssi;
      database[i].count++;
      return;
    }
  }
  if(cnt >= DB_SIZE) {
      ESP_LOGE(TAG, "Database is full!");
      return;
  }
  int pos = cnt++;
  memcpy(database[pos].mac_address, mac_address, 6);
  memcpy(database[pos].proximity_uuid,proximity_uuid,DB_UUID_LENGTH_IN_BYTE);
  database[pos].min_rssi = rssi;
  database[pos].max_rssi = rssi;
  database[pos].total_rssi = rssi;
  database[pos].count = 1;
  database[pos].remoteRssi = remoteRssi;
  database[pos].major = major;
  database[pos].minor = minor;
  database[pos].isBeacon = isBeacon;
}

/**
Dumps database to pointer
*/
char *db_dump_flush(char *timestmp) {
  cJSON *devices = cJSON_CreateObject();
  cJSON *data = cJSON_CreateArray();
  char uuid_str[48];
  char mac_str[48];



  uint8_t uuid[] = ESP_UUID;
  uuid2str(uuid, uuid_str);
  cJSON_AddItemToObject(devices, "uuid", cJSON_CreateString(uuid_str));
  cJSON_AddItemToObject(devices, "major", cJSON_CreateNumber(get_ble_major()));
  cJSON_AddItemToObject(devices, "minor", cJSON_CreateNumber(get_ble_minor()));
  cJSON_AddItemToObject(devices, "timestamp", cJSON_CreateString(timestmp));
  cJSON_AddItemToObject(devices, "comment", cJSON_CreateString(get_device_comment()));
  cJSON_AddItemToObject(devices, "interval", cJSON_CreateNumber(get_ble_scan_interval()));

  cJSON_AddItemToObject(devices, "data", data);

  for(int i = 0; i < cnt; i++){
    cJSON *beacon = cJSON_CreateObject();
    cJSON_AddItemToObject(beacon, "min", cJSON_CreateNumber(database[i].min_rssi));
    cJSON_AddItemToObject(beacon, "max", cJSON_CreateNumber(database[i].max_rssi));
    cJSON_AddItemToObject(beacon, "avg", cJSON_CreateNumber(database[i].total_rssi /  database[i].count));
    cJSON_AddItemToObject(beacon, "remoteRssi", cJSON_CreateNumber(database[i].remoteRssi));
    if(database[i].isBeacon){
      uuid2str(database[i].proximity_uuid, uuid_str);
      cJSON_AddItemToObject(beacon, "major", cJSON_CreateNumber(database[i].major));
      cJSON_AddItemToObject(beacon, "minor", cJSON_CreateNumber(database[i].minor));
      cJSON_AddItemToObject(beacon, "uuid", cJSON_CreateString(uuid_str));
    }
    mac2str(database[i].mac_address, mac_str);
    cJSON_AddItemToObject(beacon, "mac", cJSON_CreateString(mac_str));

    cJSON_AddItemToArray(data, beacon);
  }

  cnt = 0;
  char *string = cJSON_Print(devices);
  cJSON_Delete(devices);
  return string;
}
