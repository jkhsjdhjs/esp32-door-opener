/* MQTT over SSL Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "tcpip_adapter.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"

#include "mqtt.h"

#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t broker_cert_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t broker_cert_pem_start[]   asm("_binary_broker_cert_pem_start");
#endif
extern const uint8_t broker_cert_pem_end[]   asm("_binary_broker_cert_pem_end");

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    ((mqtt_callback_t) handler_args)(event);
}

esp_mqtt_client_handle_t mqtt_init(mqtt_callback_t connected_callback, mqtt_callback_t subscribed_callback, mqtt_callback_t published_callback, mqtt_callback_t message_callback) {
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_MQTT_URI,
        .cert_pem = (const char*) broker_cert_pem_start,
        .client_id = CONFIG_HOSTNAME
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, MQTT_EVENT_CONNECTED, mqtt_event_handler, connected_callback);
    esp_mqtt_client_register_event(client, MQTT_EVENT_SUBSCRIBED, mqtt_event_handler, subscribed_callback);
    esp_mqtt_client_register_event(client, MQTT_EVENT_PUBLISHED, mqtt_event_handler, published_callback);
    esp_mqtt_client_register_event(client, MQTT_EVENT_DATA, mqtt_event_handler, message_callback);
    return client;
}
