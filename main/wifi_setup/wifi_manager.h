#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"


void wifi_init_sta_UNI(void);

void wifi_init_sta_LOCAL(void);


extern esp_ip4_addr_t global_ip_address; 

extern bool wifi_connected_flag;  


#endif
