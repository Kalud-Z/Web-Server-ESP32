#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_setup/wifi_manager.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_random.h"  // Include for random number generation
#include <inttypes.h>
#include <stdlib.h>
#include "clock_synch/ntp_time.h"
#include "data_generation/data_generation.h"
#include "data_transmission/websocket.h"


const char *TAG_MAIN = "main";


void init_NVS() {
   esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}



void hello_world_task(void *pvParameter) {
    while (1) {
        if (global_ip_address.addr) {
            ESP_LOGI(TAG_MAIN, "IP Address: " IPSTR, IP2STR(&global_ip_address));
        }

        ESP_LOGI(TAG_MAIN, "Goodbye world");

        vTaskDelay(10000 / 5); 
    }
}


void app_main(void)
{
    //init_spiffs();

    //xTaskCreate(&hello_world_task, "hello_world_task", 2048, NULL, 5, NULL);

    init_NVS();

    wifi_init_sta();

    while (1) {
        if (wifi_connected_flag) {
            start_webserver();
            obtain_time();
            break; 
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


