#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "esp_random.h"  // Include for random number generation


const char *TAG_MAIN = "main";
httpd_handle_t server = NULL; // Declare the server handle globally


void hello_world_task(void *pvParameter) {
    while (1) {
        if (global_ip_address.addr) {
            ESP_LOGI(TAG_MAIN, "IP Address: " IPSTR, IP2STR(&global_ip_address));
        }

        ESP_LOGI(TAG_MAIN, "Goodbye world 5555");

        vTaskDelay(10000 / 5); 
    }
}


// Function to generate random numbers
int generate_random_number() {
    return esp_random() % 4001 + 1000; // Generate random number between 1000 and 5000
}


static esp_err_t ws_handler(httpd_req_t *req) {
    ESP_LOGI(TAG_MAIN, "WebSocket Connection Started");

    char num_str[10];
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    for (int i = 0; i < 30; i++) { // Send 10 numbers as an example
        int random_number = generate_random_number();
        sprintf(num_str, "%d", random_number); // Convert number to string
        ws_pkt.payload = (uint8_t *)num_str;
        ws_pkt.len = strlen(num_str);

        esp_err_t ret = httpd_ws_send_frame(req, &ws_pkt);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG_MAIN, "Error sending frame: %s", esp_err_to_name(ret));
            return ESP_FAIL;
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }

    ESP_LOGI(TAG_MAIN, "WebSocket Connection Closed");
    return ESP_OK;
}




void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 8999; // Set the port to 8999

    // Start the httpd server
    //httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG_MAIN, "Server started on port %d", config.server_port);

        // URI for WebSocket
        httpd_uri_t ws_uri = {
            .uri        = "/ws",  // WebSocket endpoint
            .method     = HTTP_GET,
            .handler    = ws_handler,
            .user_ctx   = NULL,
            .is_websocket = true
        };

        // Register WebSocket handler
        httpd_register_uri_handler(server, &ws_uri);
    }
}



void init_NVS() {
   esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}



void app_main(void)
{
    //init_spiffs();

    xTaskCreate(&hello_world_task, "hello_world_task", 2048, NULL, 5, NULL);

    init_NVS();


    wifi_init_sta();

    while (1) {
        if (wifi_connected_flag) {
            start_webserver();
            break; 
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}
