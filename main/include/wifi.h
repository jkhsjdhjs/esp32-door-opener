#include "esp_event.h"

void wifi_start(void (*got_ip_callback)(ip_event_got_ip_t*), void (*disconnected_callback)(void));
