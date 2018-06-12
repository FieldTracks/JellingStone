/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COLi along with JellingStone.
    If not, please contact info@fieldtracks.org

*/
#include <string.h>
#include "db.h"
#include "esp_system.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_ibeacon_api.h"
#include "cJSON.h"
#include "util.h"

#define BD_ADDR_LEN     (6)

typedef struct _db_entry {
    esp_bd_addr_t peer;
    int min_rssi;
    int max_rssi;
    int total_rssi;
    int count;
    int remoteRssi;
    uint16_t major;
    uint16_t minor;
    uint8_t proximity_uuid [16];
} db_entry;

static db_entry database[500];
static int cnt = 0;

void db_add(esp_bd_addr_t *bda, int rssi, int remoteRssi, uint16_t major, uint16_t minor, uint8_t *proximity_uuid){
  for(int i = 0; i < cnt;i++){
    if(memcmp(bda,database[i].peer,BD_ADDR_LEN) == 0) {
      database[i].min_rssi = (database[i].min_rssi < rssi) ? database[i].min_rssi : rssi;
      database[i].max_rssi = (database[i].max_rssi > rssi) ? database[i].max_rssi : rssi;
      database[i].total_rssi += rssi;
      database[i].count++;
      return;
    }
  }
  int pos =  cnt++;
  for (int i = 0; i < 16; i++) {
    database[pos].proximity_uuid[i] = (proximity_uuid != NULL) ? proximity_uuid[i] : 0;;
  }
  database[pos].min_rssi = rssi;
  database[pos].max_rssi = rssi;
  database[pos].total_rssi = rssi;
  database[pos].count = 1;
  database[pos].remoteRssi = remoteRssi;
  database[pos].major = major;
  database[pos].minor = minor;
  memcpy(&database[pos].peer,bda,BD_ADDR_LEN);
}

/**
Dumps database to pointer
*/
char *db_dump_flush(char *timestmp) {
  cJSON *devices = cJSON_CreateObject();
  char mac_str[18];
  char uuid_str[48];

  for(int i = 0; i < cnt; i++){
    mac2str(&(database[i].peer), mac_str);
    uuid2str(database[i].proximity_uuid, uuid_str);
    cJSON *beacon = cJSON_CreateObject();
    cJSON_AddItemToObject(beacon, "min", cJSON_CreateNumber(database[i].min_rssi));
    cJSON_AddItemToObject(beacon, "max", cJSON_CreateNumber(database[i].max_rssi));
    cJSON_AddItemToObject(beacon, "avg", cJSON_CreateNumber(database[i].total_rssi /  database[i].count));
    cJSON_AddItemToObject(beacon, "remoteRssi", cJSON_CreateNumber(database[i].remoteRssi));
    cJSON_AddItemToObject(beacon, "major", cJSON_CreateNumber(database[i].major));
    cJSON_AddItemToObject(beacon, "minor", cJSON_CreateNumber(database[i].minor));
    cJSON_AddItemToObject(beacon, "uuid", cJSON_CreateString(uuid_str));
    cJSON_AddItemToObject(devices, mac_str,beacon);
  }
  cJSON_AddItemToObject(devices, "timestmp", cJSON_CreateString(timestmp));
  cnt = 0;
  char *string = cJSON_Print(devices);
  cJSON_Delete(devices);
  return string;

}
