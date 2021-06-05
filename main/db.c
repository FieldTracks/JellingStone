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

typedef struct _db_entry {
    uint8_t mac_address[6];
    uint8_t network_id[DB_NETWORK_ID_LENGTH_IN_BYTE];
    uint8_t beacon_id[DB_BEACON_ID_LENGTH_IN_BYTE];
    int min_rssi;
    int max_rssi;
    int total_rssi;
    int count;
    int is_beacon;
} db_entry;

#define DB_SIZE 400

static char *TAG ="db.c";


static db_entry database[DB_SIZE];
static int cnt = 0;

void db_add_mac(uint8_t *mac_address, int rssi) {
  uint8_t *network_id = NULL;
  uint8_t *beacon_id = NULL;
  db_add_eddystone_uid(mac_address,rssi,network_id, beacon_id);

}

void db_add_eddystone_uid(uint8_t *mac_address, int rssi, uint8_t *network_id,  uint8_t *beacon_id) {
  uint8_t is_beacon = (network_id != NULL && beacon_id != NULL);
  char network_str[30];
  char id_str[18];
  char mac_str[18];

  mac2str(mac_address, mac_str);
  if(is_beacon) {
    id10bytes2str(network_id,network_str);
    mac2str(beacon_id, id_str);
    ESP_LOGI(TAG, "Adding Beacon: Network %s, Id: %s, Mac: %s", network_str, id_str, mac_str );
  } else {
    ESP_LOGI(TAG, "Adding Non-Beacon: Mac: %s", mac_str);
  }

  for(int i = 0; i < cnt;i++){
    uint8_t match = 0;

    if(memcmp(mac_address, database[i].mac_address, 6) == 0) {
      match = 1; // Case: Same MAC-Address. May occur for beacon and non-beacon advertisments

    } else if(is_beacon && memcmp(network_id,database[i].network_id,DB_NETWORK_ID_LENGTH_IN_BYTE) == 0 && memcmp(beacon_id,database[i].beacon_id,DB_BEACON_ID_LENGTH_IN_BYTE) == 0) {
      match = 1; // Case: Beacon has different MAC now. Could be changed for privacy reasons
    }

    if(match){
      if(is_beacon) {
        memcpy(database[i].network_id,network_id,DB_NETWORK_ID_LENGTH_IN_BYTE); // Update data, if the previous advertisment does not contain beacon-data
        memcpy(database[i].beacon_id,beacon_id,DB_BEACON_ID_LENGTH_IN_BYTE);
        memcpy(database[i].mac_address, mac_address, 6); // Update MAC-Address, if it has changed for privacy reason
        database[i].is_beacon = 1;
      }
      database[i].min_rssi = (database[i].min_rssi < rssi) ? database[i].min_rssi : rssi; // Update RSSI-data for statistics
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
  if(is_beacon) {
    memcpy(database[pos].network_id,network_id,DB_NETWORK_ID_LENGTH_IN_BYTE);
    memcpy(database[pos].beacon_id,beacon_id,DB_BEACON_ID_LENGTH_IN_BYTE);
  }
  ESP_LOGE(TAG, "Adding device at position %d",pos);

  database[pos].min_rssi = rssi;
  database[pos].min_rssi = rssi;
  database[pos].max_rssi = rssi;
  database[pos].total_rssi = rssi;
  database[pos].count = 1;
  database[pos].is_beacon = is_beacon;
}

/**
Dumps database to pointer
*/
char *db_dump_flush(char *timestmp, uint8_t *own_ble_mac_address) {
  cJSON *devices = cJSON_CreateObject();
  cJSON *data = cJSON_CreateArray();
  char network_str[30];
  char id_str[18];
  char mac_str[18];
  mac2str(own_ble_mac_address, mac_str);

  cJSON_AddItemToObject(devices, "timestamp", cJSON_CreateString(timestmp));
  cJSON_AddItemToObject(devices, "comment", cJSON_CreateString(get_device_comment()));
  cJSON_AddItemToObject(devices, "interval", cJSON_CreateNumber(get_ble_scan_interval()));
  cJSON_AddItemToObject(devices, "mac", cJSON_CreateString(mac_str));

  cJSON_AddItemToObject(devices, "data", data);

  for(int i = 0; i < cnt; i++){
    mac2str(database[i].mac_address, mac_str);

    cJSON *beacon = cJSON_CreateObject();
    cJSON_AddItemToObject(beacon, "min", cJSON_CreateNumber(database[i].min_rssi));
    cJSON_AddItemToObject(beacon, "max", cJSON_CreateNumber(database[i].max_rssi));
    cJSON_AddItemToObject(beacon, "avg", cJSON_CreateNumber(database[i].total_rssi /  database[i].count));
    cJSON_AddItemToObject(beacon, "mac", cJSON_CreateString(mac_str));
    if(database[i].is_beacon){
      mac2str(database[i].beacon_id, id_str);
      id10bytes2str(database[i].beacon_id, network_str);
      cJSON_AddItemToObject(beacon, "network_id", cJSON_CreateString(network_str));
      cJSON_AddItemToObject(beacon, "beacon_id", cJSON_CreateString(id_str));
    }

    cJSON_AddItemToArray(data, beacon);
  }

  cnt = 0;
  char *string = cJSON_Print(devices);
  cJSON_Delete(devices);
  return string;
}
