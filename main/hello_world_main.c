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

#include "esp_system.h"


const char *TAG_MAIN = "main";





static httpd_handle_t server = NULL;




static esp_err_t websocket_handler(httpd_req_t *req) {
    httpd_ws_frame_t ws_pkt;
    uint8_t buf[128] = {0};
    ws_pkt.payload = buf;
    int ret;

    // Logging the Content-Length and WebSocket Extensions
    char content_len[10];
    httpd_req_get_hdr_value_str(req, "Content-Length", content_len, sizeof(content_len));
    ESP_LOGI(TAG_MAIN, "Content-Length: %s", content_len);

    char ws_extensions[128] = {0};
    httpd_req_get_hdr_value_str(req, "Sec-WebSocket-Extensions", ws_extensions, sizeof(ws_extensions));
    ESP_LOGI(TAG_MAIN, "WebSocket Extensions: %s", ws_extensions);
    
    // Check if this is a new client connecting using the presence of the "Sec-WebSocket-Key" header
    size_t buf_len = httpd_req_get_hdr_value_len(req, "Sec-WebSocket-Key") + 1;
    if (buf_len > 1) {
        const char *welcome_msg = "this is a data via websocket";
        ws_pkt.len = strlen(welcome_msg);
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        ws_pkt.payload = (uint8_t *)welcome_msg;
        httpd_ws_send_frame(req, &ws_pkt);
        return ESP_OK;
    }

    // Existing client logic (for echo)
    ret = httpd_ws_recv_frame(req, &ws_pkt, sizeof(buf));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "WebSocket receive error: %d", ret);
        return ESP_FAIL;
    } else if (ws_pkt.len == 0) {
        // Connection closed by client
        return ESP_OK;
    } else {
        if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
            // Echo the same message back
            httpd_ws_send_frame(req, &ws_pkt);
        }
    }

    return ESP_OK;
}




static void start_websocket_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the HTTP Server
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t ws = {
            .uri       = "/ws",
            .method    = HTTP_GET,
            .handler   = websocket_handler,
            .user_ctx  = NULL,
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &ws);
    }
}













void hello_world_task(void *pvParameter) {
    while (1) {
        if (global_ip_address.addr) {
            ESP_LOGI(TAG_MAIN, "IP Address: " IPSTR, IP2STR(&global_ip_address));
        }

        ESP_LOGI(TAG_MAIN, "Goodbye world 22222");

        vTaskDelay(1000 / 2); 
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
    xTaskCreate(&hello_world_task, "hello_world_task", 2048, NULL, 5, NULL);

    init_NVS();


    wifi_init_sta();

    while (1) {
        if (wifi_connected_flag) {
            start_websocket_server();
            break; 
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}

