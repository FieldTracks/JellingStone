/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#include <esp_log.h>
#include "js_db.h"
#include <string.h>
#include <time.h>
#include <lwip/def.h>
#include "js_fsm.h"
#include "js_ble.h"
#include "js_mqtt.h"

static uint8_t next_free_slot = 0;
static uint8_t next_slot_for_submission = 0;

static char *TAG="js_db";

// Optimized function for comparing
static inline uint32_t my_cmp(const uint8_t *a, const uint8_t *b, int len);

// Note concurrency: This method is solely called by the BLE event handler
// Hence, we don't need to block. All data is processed one by one
// Appended data is only visible after writing, because the offset is moved in the very end
void js_db_store_beacon(uint8_t *data, int8_t detected_rssi, js_ble_beacon_t type) {
    ESP_LOGI(TAG, "Storing beacon in database. Type: %d, RSSI: %d",type, detected_rssi);
    uint8_t data_len = payload_length_of_type(type);

    // Does this beacon exists in the database?
    db_entry_t *entry = NULL;
    for (uint8_t i = next_slot_for_submission; i != next_free_slot; ) {
        db_entry_t *current = &js_db_database[i];
        if (current->type == type && my_cmp(data, current->data, data_len) == 0) {
            ESP_LOGI(TAG, "Beacon exists in database");
            entry = current;
            break;
        }
        i++; // Overflow at database-max (i.e. 255)
    }
    // If the entry was found: Update RSSI, done
    if (entry != NULL) {
        ESP_LOGI(TAG, "Updating entry");
        if(entry->max_rssi < detected_rssi) {
            entry->max_rssi = detected_rssi;
        }
    } else { // Create a new database entry
        // Note: It could be the case that there's one slot left, still.
        // Strategy: Use it for this entry, and report db-full later on.
        ESP_LOGI(TAG, "Beacon not found in database-creating new entry at position: %d", next_free_slot);
        entry = &js_db_database[next_free_slot];
        memcpy(entry->data, data, data_len);
        entry->max_rssi = detected_rssi;
        entry->type = type;
        // Note Concurrency: Update marker at end to avoid submitting incomplete results in a different task
        uint8_t slot = next_free_slot + 1; // Overflow at database-max (i.e. 255)

        // Note: Atomic Update
        next_free_slot = slot;

        if (next_free_slot == next_slot_for_submission) {
            ESP_LOGI(TAG, "Database is full");
            js_on_db_full();
        }

    }
}

// Helper for easier code
static inline void insert_timestamp(long *position) {
    time_t timestamp;
    if(sizeof(timestamp) != 4) {
        ESP_LOGE(TAG, "time_t is not 32-Bit. This protocol requires a 32-Bit time_t. Expect overflows closer to the year 2038.");
    }
    time(&timestamp);
    uint32_t converted = htonl(timestamp);
    *position = converted;
}

// Wire-protocol, sent using MQTT
// 0. byte: protocol version: this protocol: 0x01, JSON-Report: 0x7B
// 1 .. 4. byte: timestamp (32-Bit, 4 Bytes)
// 5. byte message_id (signed)
// 6.byte number of beacons in this message (0 ... 255)
// 7. byte ... 999 byte: Beacon data

#define REPORT_HEADER_SIZE_IN_BYTES 7
#define REPORT_VERSION_POS 0
#define REPORT_TIMESTMP_POS 1
#define REPORT_MID_POS 5
#define REPORT_BNUM_POS 6

// Beacon-data:
// 0. byte: Beacon Type (c.f. enum)
// 1. byte: max_rssi + 100 - Note: Because 2.4 GHz transmission is always below 24 dBm, this shifts the byte into a safe areay
// 3. byte ... actual beacon data (Note: The length depends on the type)

#define REPORT_BEACON_HEADER_SIZE_IN_BYTES 2
#define DATA_TYPE_POS 0
#define DATA_RSSI_POS 1
#define DATA_BEACON_DATA_POS 2

static uint8_t m_buffer[1000]; // No more than 1000 Bytes per Message over MQTT => Avoid fragmentation on lower layers
error_t js_db_submit_over_mqtt() {
    ESP_LOGI(TAG, "Submitting database over MQTT");
    ESP_LOGI(TAG, "Preparing new message");
    m_buffer[REPORT_VERSION_POS] = 1; // Protocol-Version: 1
    insert_timestamp((long *) &m_buffer[REPORT_TIMESTMP_POS]); // Timestamp - 32-Bit time_t on ESP-IDF. MIND the year 2038 or changes in esp-idf
    m_buffer[REPORT_MID_POS] = 1; // Message-ID: 1, first message
    m_buffer[REPORT_BNUM_POS] = 0;
    uint16_t bytes_written = REPORT_HEADER_SIZE_IN_BYTES;
    ESP_LOGI(TAG, "Preparing new message - added %d bytes of header", bytes_written);

    int msgid = 0;
    while(next_slot_for_submission != next_free_slot){
        db_entry_t *entry = &js_db_database[next_slot_for_submission];
        uint8_t * d_pos = &m_buffer[bytes_written];
        d_pos[DATA_TYPE_POS] = entry->type;
        d_pos[DATA_RSSI_POS] = entry->max_rssi + 100;
        uint8_t data_len = payload_length_of_type(entry->type); // easy for now

        memcpy(&d_pos[DATA_BEACON_DATA_POS],entry->data,data_len);
        bytes_written += data_len + REPORT_BEACON_HEADER_SIZE_IN_BYTES;
        m_buffer[REPORT_BNUM_POS]++;
        next_slot_for_submission = next_slot_for_submission + 1; // Overflow at 255 (i.e. database-size)
        ESP_LOGI(TAG, "Added entry - type: %d, rssi: %d", d_pos[DATA_TYPE_POS], d_pos[DATA_RSSI_POS]);

        if((bytes_written >= 1024 || m_buffer[REPORT_BNUM_POS] == 255)  && next_slot_for_submission != next_free_slot) { // Less than one alt-beacon free, still data to submit
            ESP_LOGI(TAG, "Message has %d bytes and exceeds limit of 1079 bytes - preparing next one", bytes_written);
            js_mqtt_publish_report(m_buffer, bytes_written,&msgid);
            // Re-use message-buffer for the next-message
            m_buffer[REPORT_MID_POS]++;
            m_buffer[REPORT_BNUM_POS]=0;
            bytes_written = REPORT_HEADER_SIZE_IN_BYTES;
        }
    }
    // Send last message, close the report
    m_buffer[REPORT_MID_POS] *= -1;
    ESP_LOGI(TAG, "Submitting final message - closing report");
    return js_mqtt_publish_report(m_buffer, bytes_written,&msgid);

}
void js_db_clear() {
    ESP_LOGI(TAG, "Reset database");
    next_free_slot = 0;
    next_slot_for_submission = 0;
}

static inline uint32_t my_cmp(const uint8_t *a, const uint8_t *b, const int len) {
    switch (len) {
        case 0:
            return 0;
        case 1:
            return *a != *b;
        case 2:
            return * (uint16_t*) a != * (uint16_t*) b;
        case 4:
            return * (uint32_t *) a != * (uint32_t*) b;
        default:
            return memcmp(a,b,len);
    }
}