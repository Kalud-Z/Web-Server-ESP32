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



const char *TAG_MAIN = "main";

void hello_world_task(void *pvParameter) {
    while (1) {
        if (global_ip_address.addr) {
            ESP_LOGI(TAG_MAIN, "IP Address: " IPSTR, IP2STR(&global_ip_address));
        }

        ESP_LOGI(TAG_MAIN, "Hello World 9999");

        vTaskDelay(1000 / 5); 
    }
}

esp_err_t hello_get_handler(httpd_req_t *req)
{
    const char* resp_str = "This is your response :)";
    httpd_resp_send(req, resp_str, strlen(resp_str)); // Send response
    return ESP_OK;
}

void start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the HTTP server
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        httpd_uri_t hello_uri = {
            .uri       = "/hello",
            .method    = HTTP_GET,
            .handler   = hello_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &hello_uri);
    }

    // Print server status
    if (server) {
        ESP_LOGI(TAG_MAIN, "HTTP server started on port %d", config.server_port);
    } else {
        ESP_LOGE(TAG_MAIN, "Failed to start HTTP server!");
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

    start_webserver();
}
