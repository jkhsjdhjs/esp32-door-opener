#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi.h"

static const char *TAG = "wifi_client";

static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if(event_base == WIFI_EVENT) {
        if(event_id == WIFI_EVENT_STA_START) {
            tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, CONFIG_HOSTNAME);
            esp_wifi_connect();
        }
        else if(event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ((void (*)(void)) arg)();
            if(s_retry_num < CONFIG_WIFI_MAXIMUM_RETRY || CONFIG_WIFI_MAXIMUM_RETRY == 0) {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "retry connecting to \"" CONFIG_WIFI_SSID "\"");
            }
            ESP_LOGI(TAG, "connecting to \"" CONFIG_WIFI_SSID "\" failed");
        }
    }
    else if(event_base == IP_EVENT) {
        if(event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
            ((void (*)(ip_event_got_ip_t*)) arg)(event);
            s_retry_num = 0;
        }
    }
    // -- IPv6 --
    /*tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
    else if(event_base == IP_EVENT && event_id == IP_EVENT_GOT_IP6) {
        ip_event_got_ip6_t *event = (ip_event_got_ip6_t*) event_data;
        ESP_LOGI(TAG, "got ipv6: " IPV6STR, IPV62STR(event->ip6_info.ip));
    }*/
}

void wifi_start(void (*got_ip_callback)(ip_event_got_ip_t*), void (*disconnected_callback)(void)) {
    tcpip_adapter_init();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event_handler, disconnected_callback));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, got_ip_callback));
    //ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}
