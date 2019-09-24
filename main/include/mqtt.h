#include "mqtt_client.h"

typedef void (*mqtt_callback_t)(esp_mqtt_event_handle_t);

esp_mqtt_client_handle_t mqtt_init(mqtt_callback_t connected_callback, mqtt_callback_t subscribed_callback, mqtt_callback_t published_callback, mqtt_callback_t message_callback);
