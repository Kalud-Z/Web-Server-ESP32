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
#include <time.h>
#include <inttypes.h>
#include <sys/time.h>  // Include for getting system time
#include "lwip/apps/sntp.h"
#include <stdlib.h>
#include "ntp_time.h"
#include "data_generation.h"
#include "esp_timer.h"



const char *TAG_MAIN = "main";


const int totalDataPoints = 10000;
const int dataPointsPerBatch = 10;
int sampleRate = 10;


httpd_handle_t server = NULL; // Declare the server handle globally



static esp_err_t ws_handler(httpd_req_t *req) {
    ESP_LOGI(TAG_MAIN, "WebSocket Connection Started");

    // Send initial configuration message
    const char* init_message = "{ \"type\": \"configuration\", \"channels\": 2 }";
    httpd_ws_frame_t init_frame;
    memset(&init_frame, 0, sizeof(httpd_ws_frame_t));
    init_frame.type = HTTPD_WS_TYPE_TEXT;
    init_frame.payload = (uint8_t*)init_message;
    init_frame.len = strlen(init_message);

    esp_err_t ret = httpd_ws_send_frame(req, &init_frame);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "Error sending initial frame: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    const int totalBatches = totalDataPoints / dataPointsPerBatch;

    for (uint32_t batchCount = 1; batchCount <= totalBatches; batchCount++) {
        int64_t startTime = esp_timer_get_time();  // Start time in microseconds

        size_t binary_size;
        uint8_t* binary_data = generate_data_batch(batchCount, &binary_size, dataPointsPerBatch);
        int64_t endTime = esp_timer_get_time();  // End time in microseconds
        int64_t generationTime = (endTime - startTime) / 1000;  // Convert to milliseconds

        if (binary_data == NULL) {
            return ESP_FAIL;
        }

        // Log the total size of the batch being sent
        //ESP_LOGI(TAG_MAIN, "Sending batchID: %" PRIu32 " | Size: %zu bytes", batchCount, binary_size);


        // Get the current time
        struct timeval tv_now;
        gettimeofday(&tv_now, NULL);
        struct tm *tm_now = localtime(&tv_now.tv_sec);

        // Format the time
        char strftime_buf[64];
        strftime(strftime_buf, sizeof(strftime_buf), "%H:%M:%S", tm_now);

        // Log the timestamp
        ESP_LOGI(TAG_MAIN, "Dispatching batchID: %" PRIu32 " | timestamp: %s:%03ld", batchCount, strftime_buf, tv_now.tv_usec / 1000);


        httpd_ws_frame_t ws_pkt;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        ws_pkt.type = HTTPD_WS_TYPE_BINARY;
        ws_pkt.payload = binary_data;
        ws_pkt.len = binary_size;

        esp_err_t ret = httpd_ws_send_frame(req, &ws_pkt);
        free(binary_data);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG_MAIN, "Error sending frame: %s", esp_err_to_name(ret));
            return ESP_FAIL;
        }

         if (generationTime > sampleRate) {
            ESP_LOGE(TAG_MAIN, "$$$$$Data generation took longer than target interval: %" PRId64 " ms", generationTime);
            // Handle the error condition here (e.g., skip sending this batch, reset, etc.)
            continue;  // Skip to the next iteration, or use a different handling approach
        }

        int finalDelayTime = sampleRate - generationTime;
        vTaskDelay(pdMS_TO_TICKS(finalDelayTime)); //TODO: Timer instead. // the logging of the timestamps proved that this clock is VERY accurate.
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



void hello_world_task(void *pvParameter) {
    while (1) {
        if (global_ip_address.addr) {
            ESP_LOGI(TAG_MAIN, "IP Address: " IPSTR, IP2STR(&global_ip_address));
        }

        ESP_LOGI(TAG_MAIN, "Goodbye world 666");

        vTaskDelay(10000 / 5); 
    }
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
            obtain_time();
            break; 
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}


