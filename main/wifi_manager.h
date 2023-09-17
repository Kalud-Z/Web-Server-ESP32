#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"

// Function to initialize WiFi as station mode and connect to the defined SSID.
void wifi_init_sta(void);

// extern EventGroupHandle_t s_wifi_event_group;
// #define WIFI_CONNECTED_BIT BIT0


extern const char *TAG;  // External declaration for the TAG variable

#endif // WIFI_MANAGER_H
