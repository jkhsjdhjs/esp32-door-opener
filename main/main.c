#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "wifi.h"
#include "mqtt.h"

#define GPIO_NUM(X) GPIO_NUM_##X
#define GPIO_NUM_EXPAND(X) GPIO_NUM(X)
#define GPIO_DOOR_OPENER GPIO_NUM_EXPAND(CONFIG_GPIO_DOOR_OPENER)

static const char *TAG = "mqtt_door_opener";

static esp_mqtt_client_handle_t mqtt_client;
static char mqtt_running = 0;

static esp_err_t open_door() {
    esp_err_t result = gpio_set_level(GPIO_DOOR_OPENER, 1);
    if(result != ESP_OK)
        return result;
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(gpio_set_level(GPIO_DOOR_OPENER, 0));
    return result;
}

static void wifi_cb_got_ip(ip_event_got_ip_t *event) {
    ESP_LOGI(TAG, "got ip: %s", ip4addr_ntoa(&event->ip_info.ip));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
    mqtt_running = 1;
}

static void wifi_cb_disconnected(void) {
    ESP_LOGI(TAG, "disconnected from wifi");
    if(mqtt_running) {
        ESP_LOGI(TAG, "stopping mqtt client...");
        ESP_ERROR_CHECK(esp_mqtt_client_stop(mqtt_client));
        mqtt_running = 0;
    }
}

static void mqtt_cb_connected(esp_mqtt_event_handle_t event) {
    ESP_LOGI(TAG, "connected to mqtt broker");
    assert(esp_mqtt_client_subscribe(event->client, CONFIG_MQTT_TOPIC, 2) != -1);
}

static void mqtt_cb_subscribed(esp_mqtt_event_handle_t event) {
    ESP_LOGI(TAG, "subscribed to \"" CONFIG_MQTT_TOPIC "\"");
}

static void mqtt_cb_published(esp_mqtt_event_handle_t event) {
    ESP_LOGI(TAG, "published message with id %d", event->msg_id);
}

static void mqtt_cb_message(esp_mqtt_event_handle_t event) {
    if(event->data_len != event->total_data_len) {
        ESP_LOGW(TAG, "arrived message was too long. ignoring...");
        return;
    }
    ESP_LOGI(TAG, "message arrived on topic \"%.*s\": \"%.*s\"", event->topic_len, event->topic, event->data_len, event->data);
    char message[5];
    sprintf(message, "%.*s", 4, event->data);
    if(event->data_len == 4 && strcmp(message, CONFIG_MQTT_OPEN_COMMAND) == 0) {
        ESP_LOGI(TAG, "opening door...");
        if(open_door() != ESP_OK)
            ESP_LOGE(TAG, "failed to open door");
        else
            ESP_LOGI(TAG, "closed door");
    }
}

void app_main() {
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(gpio_set_direction(GPIO_DOOR_OPENER, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_DOOR_OPENER, 0));

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    mqtt_client = mqtt_init(&mqtt_cb_connected, &mqtt_cb_subscribed, &mqtt_cb_published, &mqtt_cb_message);

    ESP_LOGI(TAG, "connecting to \"" CONFIG_WIFI_SSID "\"");
    wifi_start(&wifi_cb_got_ip, &wifi_cb_disconnected);
}
