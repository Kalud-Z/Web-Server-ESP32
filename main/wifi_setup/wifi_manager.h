#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"

// Function to initialize WiFi as station mode and connect to the defined SSID.
void wifi_init_sta(void);

extern esp_ip4_addr_t global_ip_address; 

extern bool wifi_connected_flag;  // Flag to indicate WiFi connection status


extern const char *TAG;  // External declaration for the TAG variable

#endif
