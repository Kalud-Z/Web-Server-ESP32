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


const char *data = 
"87780270000ns 751 555\n"
"87800417000ns 750 557\n"
"87820270000ns 747 555\n"
"87840456000ns 752 560\n"
"87860609000ns 749 553\n"
"87880405000ns 749 554\n"
"87900552000ns 753 555\n"
"87920694000ns 751 559\n"
"87940495000ns 752 556\n"
"87960642000ns 751 555\n"
"87980783000ns 746 553\n"
"88000933000ns 750 557\n"
"88020784000ns 751 555\n"
"88040973000ns 749 558\n"
"88060783000ns 751 559\n"
"88080932000ns 750 553\n"
"88101078000ns 749 560\n"
"88120882000ns 752 558\n"
"88141023000ns 749 556\n"
"88161171000ns 751 558\n"
"88181312000ns 750 553\n"
"88201118000ns 747 558\n";





void hello_world_task(void *pvParameter) {
    while (1) {
        if (global_ip_address.addr) {
            ESP_LOGI(TAG_MAIN, "IP Address: " IPSTR, IP2STR(&global_ip_address));
        }

        ESP_LOGI(TAG_MAIN, "Goodbye world 1111");

        vTaskDelay(1000 / 5); 
    }
}


const char* get_line(const char* data, int line_number) {
    static char buffer[256];
    char data_copy[1024]; // Assuming 1024 is enough
    strncpy(data_copy, data, sizeof(data_copy) - 1);
    data_copy[sizeof(data_copy) - 1] = '\0'; // Null-terminate just in case

    int current_line = 0;
    char* token = strtok(data_copy, "\n");
    while (token) {
        if (current_line == line_number) {
            strncpy(buffer, token, sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
            return buffer;
        }
        token = strtok(NULL, "\n");
        current_line++;
    }
    return NULL; // Line not found
}



esp_err_t hello_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG_MAIN, "hello_get_handler just called._______________________________");  

    char response[1024] = {0};
    for (int i = 0; i < 10; i++) {
        const char *line = get_line(data, i);
        if(line) {
            strncat(response, line, sizeof(response) - strlen(response) - 1);
            strncat(response, "\n", sizeof(response) - strlen(response) - 1);
        }
    }
    
    ESP_LOGI(TAG_MAIN, "this is response sent to client : %s", response);  

    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}


// esp_err_t hello_get_handler(httpd_req_t *req)
// {
//     ESP_LOGI(TAG_MAIN, "hello_get_handler just called.");  

//     const char* resp_str = "This is your response :)";

//     ESP_LOGI(TAG_MAIN, "this is response sent to client : %s", resp_str);  

//     httpd_resp_send(req, resp_str, strlen(resp_str)); 
//     return ESP_OK;
// }



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


// void init_spiffs() {
//     esp_vfs_spiffs_conf_t conf = {
//       .base_path = "/spiffs",
//       .partition_label = NULL,
//       .max_files = 5,
//       .format_if_mount_failed = true
//     };

//     esp_err_t ret = esp_vfs_spiffs_register(&conf);

//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG_MAIN, "Failed to initialize SPIFFS");
//         return;
//     }
// }



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
