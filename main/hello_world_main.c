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


void hello_world_task(void *pvParameter) {
    while (1) {
        if (global_ip_address.addr) {
            ESP_LOGI(TAG_MAIN, "IP Address: " IPSTR, IP2STR(&global_ip_address));
        }

        ESP_LOGI(TAG_MAIN, "Goodbye world 5555");

        vTaskDelay(1000 / 5); 
    }
}



static esp_err_t ws_handler(httpd_req_t *req)
{
    uint8_t buf[128] = { 0 };
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = buf;

    // Receive the WebSocket message
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 128);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "Error receiving the frame");
        return ESP_FAIL;
    }

    // Handle the WebSocket message (echo in this case)
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT &&
        ws_pkt.len < 128) {
        buf[ws_pkt.len] = '\0';  // Null-terminate the received string
        ESP_LOGI(TAG_MAIN, "WebSocket Message: %s", buf);
        // Echo the message back
        httpd_ws_send_frame(req, &ws_pkt);
    }

    return ESP_OK;
}



void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG_MAIN, "Server started");

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
            //start_webserver();
            break; 
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}
