// websocket.c
#include "websocket.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include "../data_generation/data_generation.h" 
#include <inttypes.h>
#include "esp_heap_caps.h"



const char *TAG_WS = "websocket";


//int64_t durationOfSimulation = 30000000; // 30 seconds in microseconds
int64_t durationOfSimulation = 300000000; // 5 minutes seconds in microseconds
const int numberOfChannels = 10;
const int dataPointsPerBatch = 50;
const int sampleRate = 50;  


volatile size_t total_size_sent = 0;
volatile bool sending_done = false; 
volatile size_t total_data_points_sent = 0;

volatile size_t size_sent_per_second = 0;
volatile size_t total_size_per_second = 0; // Total of data sent per second
volatile int count_of_data_points = 0; // Count of data sent per second records


httpd_handle_t server = NULL;




void monitor_ram_usage() {
    int total_ram = heap_caps_get_total_size(MALLOC_CAP_8BIT);
    int free_ram = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    int used_ram = total_ram - free_ram;
    float ram_usage_percent = ((float)used_ram / total_ram) * 100;

    ESP_LOGI(TAG_WS, "RAM Usage: %.2f%%", ram_usage_percent);
}


void log_data_size_per_second(void *pvParameter) {
    while (!sending_done) {
        ESP_LOGI(TAG_WS, "Data Sent in Last Second: %zu bytes", size_sent_per_second);
        if (size_sent_per_second > 0) {
            total_size_per_second += size_sent_per_second;
            count_of_data_points++;
        }
        size_sent_per_second = 0;
        monitor_ram_usage();
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
    }
    vTaskDelete(NULL);
}



esp_err_t send_initial_config_message(httpd_req_t *req) {
    char init_message[64];
    snprintf(init_message, sizeof(init_message), "{ \"type\": \"configuration\", \"channels\": %d }", numberOfChannels);
    httpd_ws_frame_t init_frame;
    memset(&init_frame, 0, sizeof(httpd_ws_frame_t));
    init_frame.type = HTTPD_WS_TYPE_TEXT;
    init_frame.payload = (uint8_t*)init_message;
    init_frame.len = strlen(init_message);

    return httpd_ws_send_frame(req, &init_frame);
}


esp_err_t send_end_simulation_message(httpd_req_t *req) {
    char done_message[64];
    snprintf(done_message, sizeof(done_message), "{ \"type\": \"simulationState\", \"value\": \"DONE\" }");
    httpd_ws_frame_t done_frame;
    memset(&done_frame, 0, sizeof(httpd_ws_frame_t));
    done_frame.type = HTTPD_WS_TYPE_TEXT;
    done_frame.payload = (uint8_t*)done_message;
    done_frame.len = strlen(done_message);

    return httpd_ws_send_frame(req, &done_frame);
}


esp_err_t run_simulation(httpd_req_t *req) {
    int64_t startSendingTime = esp_timer_get_time();
    uint32_t batchCount = 0;

    while (esp_timer_get_time() - startSendingTime < durationOfSimulation) {
        int64_t iterationStartTime = esp_timer_get_time();

        // Variables to store start and end times of tasks
        int64_t startGenTime, endGenTime, startSendTime, endSendTime;

        batchCount++;


        size_t binary_size;
        startGenTime = esp_timer_get_time();
        uint8_t* binary_data = generate_data_batch(batchCount, &binary_size, dataPointsPerBatch, numberOfChannels);
        endGenTime = esp_timer_get_time();
        if (binary_data == NULL) { return ESP_FAIL;}

        total_data_points_sent += dataPointsPerBatch;

        // Use gettimeofday to get current time
        struct timeval tv;
        gettimeofday(&tv, NULL);
        uint64_t sendTimestamp = (uint64_t)(tv.tv_sec) * 1000LL + (uint64_t)(tv.tv_usec) / 1000LL; // Convert to milliseconds
        
        //ESP_LOGI(TAG_WS, "Sending Timestamp: %" PRIu64, sendTimestamp);

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

        startSendTime = esp_timer_get_time();
        esp_err_t ret = httpd_ws_send_frame(req, &ws_pkt);
        endSendTime = esp_timer_get_time();

        free(combined_data);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG_WS, "Error sending frame: %s", esp_err_to_name(ret));
            return ESP_FAIL;
        }
       
        int64_t iterationDuration = esp_timer_get_time() - iterationStartTime;

        if (iterationDuration < sampleRate * 1000) {
            vTaskDelay(pdMS_TO_TICKS(sampleRate - (iterationDuration / 1000)));
        }
        else {
            int64_t iterationDurationMs = iterationDuration / 1000;
            int64_t genDurationMs = (endGenTime - startGenTime) / 1000;
            int64_t sendDurationMs = (endSendTime - startSendTime) / 1000;

            ESP_LOGW(TAG_WS, "WARNING: Iteration duration (%lld ms) is longer than the sample rate. Batch Count: %" PRIu32, iterationDurationMs, batchCount);
            ESP_LOGW(TAG_WS, "Task A (Data Generation) took %lld ms", genDurationMs);
            ESP_LOGW(TAG_WS, "Task C (Sending Data) took %lld ms", sendDurationMs);
        }

        
    } //end of while loop

    sending_done = true;
    ESP_LOGI(TAG_WS, "Total Data Sent: %zu bytes", total_size_sent);
    ESP_LOGI(TAG_WS, "Total Data Points Sent: %zu", total_data_points_sent);

    float mean_average = (float)total_size_per_second / count_of_data_points;
    ESP_LOGI(TAG_WS, "Mean Average Data Sent Per Second: %f bytes", mean_average);
    ESP_LOGI(TAG_WS, "Total Number of Data Batches Sent: %lu", (unsigned long)batchCount);


    return ESP_OK;
}



esp_err_t ws_handler(httpd_req_t *req) {
    ESP_LOGI(TAG_WS, "WebSocket Connection Started");
    xTaskCreate(&log_data_size_per_second, "log_data_task", 2048, NULL, 5, NULL);

    esp_err_t ret1 = send_initial_config_message(req);
    if (ret1 != ESP_OK) {
        ESP_LOGE(TAG_WS, "Error sending initial frame: %s", esp_err_to_name(ret1));
        return ESP_FAIL;
    }

    esp_err_t ret2 = run_simulation(req);
    if (ret2 != ESP_OK) {
        ESP_LOGE(TAG_WS, "Simulation failed: %s", esp_err_to_name(ret2));
        return ESP_FAIL; 
    }

    esp_err_t ret3 = send_end_simulation_message(req);
    if (ret3 != ESP_OK) {
        ESP_LOGE(TAG_WS, "Error sending end frame: %s", esp_err_to_name(ret3));
        return ESP_FAIL; 
    }

    return ESP_OK;
}



void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 8999; // Set the port to 8999

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG_WS, "Server started on port %d", config.server_port);

        // URI for WebSocket
        httpd_uri_t ws_uri = { .uri = "/ws", .method = HTTP_GET, .handler = ws_handler, .user_ctx = NULL, .is_websocket = true };
        httpd_register_uri_handler(server, &ws_uri);
    }
}

