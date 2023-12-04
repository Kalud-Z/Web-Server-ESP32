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
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "esp_random.h"  // Include for random number generation
#include <time.h>
#include <inttypes.h>
#include <sys/time.h>  // Include for getting system time
#include "lwip/apps/sntp.h"
#include <stdlib.h>
#include "clock_synch/ntp_time.h"
#include "data_generation/data_generation.h"
#include "esp_timer.h"


int64_t durationOfSimulation = 30000000; // 30 seconds in microseconds
const int numberOfChannels = 10;
const int dataPointsPerBatch = 250;
const int sampleRate = 250;


const char *TAG_MAIN = "main";
httpd_handle_t server = NULL; // Declare the server handle globally


// Global variables for tracking data size
volatile size_t total_size_sent = 0;
volatile bool sending_done = false; 
volatile size_t total_data_points_sent = 0;

volatile size_t size_sent_per_second = 0;
volatile size_t total_size_per_second = 0; // Total of data sent per second
volatile int count_of_data_points = 0; // Count of data sent per second records




void log_data_size_per_second(void *pvParameter) {
    while (!sending_done) {
        ESP_LOGI(TAG_MAIN, "Data Sent in Last Second: %zu bytes", size_sent_per_second);
        if (size_sent_per_second > 0) {
            total_size_per_second += size_sent_per_second;
            count_of_data_points++;
        }
        size_sent_per_second = 0;
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
    }
    vTaskDelete(NULL);
}




static esp_err_t ws_handler(httpd_req_t *req) {
    ESP_LOGI(TAG_MAIN, "WebSocket Connection Started");
    xTaskCreate(&log_data_size_per_second, "log_data_task", 2048, NULL, 5, NULL);


    char init_message[64];
    snprintf(init_message, sizeof(init_message), "{ \"type\": \"configuration\", \"channels\": %d }", numberOfChannels);
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

    int64_t startSendingTime = esp_timer_get_time();
    uint32_t batchCount = 0;

    while (esp_timer_get_time() - startSendingTime < durationOfSimulation) {
        int64_t iterationStartTime = esp_timer_get_time();

        batchCount++;

        size_t binary_size;
        //int64_t startGenerationTime = esp_timer_get_time(); 
        uint8_t* binary_data = generate_data_batch(batchCount, &binary_size, dataPointsPerBatch, numberOfChannels);
        //int64_t dataGenerationDuration = esp_timer_get_time() - startGenerationTime; 
        if (binary_data == NULL) { return ESP_FAIL;}

        total_data_points_sent += dataPointsPerBatch;

        // Use gettimeofday to get current time
        struct timeval tv;
        gettimeofday(&tv, NULL);
        uint64_t sendTimestamp = (uint64_t)(tv.tv_sec) * 1000LL + (uint64_t)(tv.tv_usec) / 1000LL; // Convert to milliseconds
        
        //ESP_LOGI(TAG_MAIN, "Sending Timestamp: %" PRIu64, sendTimestamp);

        size_t combined_size = sizeof(sendTimestamp) + binary_size;
        uint8_t* combined_data = malloc(combined_size);
        if (combined_data == NULL) {
            free(binary_data);
            return ESP_FAIL;
        }
    
        memcpy(combined_data, &sendTimestamp, sizeof(sendTimestamp));
        memcpy(combined_data + sizeof(sendTimestamp), binary_data, binary_size);
        free(binary_data);

        httpd_ws_frame_t ws_pkt;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        ws_pkt.type = HTTPD_WS_TYPE_BINARY;
        ws_pkt.payload = combined_data;
        ws_pkt.len = combined_size;

        total_size_sent += combined_size;
        size_sent_per_second += combined_size;

        ret = httpd_ws_send_frame(req, &ws_pkt);
        free(combined_data);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG_MAIN, "Error sending frame: %s", esp_err_to_name(ret));
            return ESP_FAIL;
        }
       


        int64_t iterationDuration = esp_timer_get_time() - iterationStartTime;
        if (iterationDuration < sampleRate * 1000) {
            vTaskDelay(pdMS_TO_TICKS(sampleRate - (iterationDuration / 1000)));
        }
        
    }

    sending_done = true;
    ESP_LOGI(TAG_MAIN, "Total Data Sent: %zu bytes", total_size_sent);
    ESP_LOGI(TAG_MAIN, "Total Data Points Sent: %zu", total_data_points_sent);

    float mean_average = (float)total_size_per_second / count_of_data_points;
    ESP_LOGI(TAG_MAIN, "Mean Average Data Sent Per Second: %f bytes", mean_average);
    ESP_LOGI(TAG_MAIN, "Total Number of Data Batches Sent: %lu", (unsigned long)batchCount);



    char done_message[64];
    snprintf(done_message, sizeof(done_message), "{ \"type\": \"simulationState\", \"value\": \"DONE\" }");
    httpd_ws_frame_t done_frame;
    memset(&done_frame, 0, sizeof(httpd_ws_frame_t));
    done_frame.type = HTTPD_WS_TYPE_TEXT;
    done_frame.payload = (uint8_t*)done_message;
    done_frame.len = strlen(done_message);

    ret = httpd_ws_send_frame(req, &done_frame);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "Error sending done frame: %s", esp_err_to_name(ret));
    }


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


