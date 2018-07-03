void status_booting();

void status_set(int status);

void status_ack_sent();

#define STATUS_BOOTING 1
#define STATUS_WIFI_CONNECTED 2
#define STATUS_MQTT_CONNECTED 3
#define STATUS_WIFI_DISCONNECTED 4
#define STATUS_MQTT_DISCONNECTED 5
#define STATUS_NVS_MISSINGDATA 6
