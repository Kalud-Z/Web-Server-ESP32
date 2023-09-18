#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"

// Function to initialize WiFi as station mode and connect to the defined SSID.
void wifi_init_sta(void);

extern const char *TAG;  // External declaration for the TAG variable

#endif
