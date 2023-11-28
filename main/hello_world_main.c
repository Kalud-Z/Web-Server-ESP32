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



const char *TAG_MAIN = "main";
const char *TAG_ws = "ws_server";


void hello_world_task(void *pvParameter) {
    while (1) {
        if (global_ip_address.addr) {
            ESP_LOGI(TAG_MAIN, "IP Address: " IPSTR, IP2STR(&global_ip_address));
        }

        ESP_LOGI(TAG_MAIN, "Goodbye world 44444");

        vTaskDelay(1000 / 5); 
    }
}





/* An example of WebSocket handler */
static esp_err_t websocket_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG_ws, "Handshake done, WebSocket connection established");
        const char *response = "connection established :)";
        httpd_ws_frame_t ws_response = {
            .payload = (uint8_t *)response,
            .len = strlen(response),
            .type = HTTPD_WS_TYPE_TEXT,
            .final = true,
            .fragmented = false,  // Set fragmented to false for a complete message
        };
        httpd_ws_send_frame(req, &ws_response);
        return ESP_OK;
    }
    // Handle other WebSocket events here
    return ESP_FAIL;
}



void start_webserver(void)
{
  httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG_ws, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Register the WebSocket handler
        httpd_uri_t ws_uri = {
            .uri       = "/ws", // The WebSocket endpoint
            .method    = HTTP_GET,
            .handler   = websocket_handler,
            .user_ctx  = NULL
        };
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
            //start_webserver();
            break; 
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}
