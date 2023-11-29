#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"        
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
#include "esp_timer.h"
#include <inttypes.h>

// Global variable to store the WebSocket request handle
static httpd_req_t *global_req = NULL;



typedef struct {
    uint64_t timestamp;       // Timestamp
    uint32_t channel1Value;   // Channel 1 value
    uint32_t channel2Value;   // Channel 2 value
    uint32_t dataPointID;     // Data point ID
} DataPoint;

typedef struct {
    uint32_t batchID;
    DataPoint dataPoints[2]; // Array of 2 DataPoints
} DataBatch;




const char *TAG_MAIN = "main";
httpd_handle_t server = NULL; // Declare the server handle globally





void timerCallback(TimerHandle_t xTimer) {
    static uint32_t batchCount = 1; // Static variable to track batch count


    // Check if the WebSocket connection is still valid
    if (global_req == NULL) {
        ESP_LOGE(TAG_MAIN, "inside timerCallback - WebSocket connection not established");
        return;
    }
     else {
        ESP_LOGI(TAG_MAIN, "global_req is valid in timerCallback");
    }

    ESP_LOGI(TAG_MAIN, "global_req set in ws_handler: %p", (void*)global_req);


    DataBatch batch;
    
    batch.batchID = batchCount++; // Increment batch ID

    uint64_t currentTime = esp_timer_get_time();

    // Populate data points with incremented timestamps
    for (int i = 0; i < 2; i++) {
        batch.dataPoints[i].timestamp = currentTime + (i * 1000); // 1 millisecond interval between data points
        batch.dataPoints[i].channel1Value = 50 + (batchCount - 1) * 10; 
        batch.dataPoints[i].channel2Value = 150 + (batchCount - 1) * 10;
        batch.dataPoints[i].dataPointID = i + 1;
    }

    // Calculate the size of the binary data
    size_t binary_size = 4 + (2 * sizeof(DataPoint));
    uint8_t* binary_data = malloc(binary_size);
    if (binary_data == NULL) {
        ESP_LOGE(TAG_MAIN, "Failed to allocate memory for binary data");
        return;
    }

    // Serialize data into binary format
    uint8_t* ptr = binary_data;
    memcpy(ptr, &batch.batchID, 4); ptr += 4;
    for (int i = 0; i < 2; i++) {
        memcpy(ptr, &batch.dataPoints[i].timestamp, 8); ptr += 8;
        memcpy(ptr, &batch.dataPoints[i].channel1Value, 4); ptr += 4;
        memcpy(ptr, &batch.dataPoints[i].channel2Value, 4); ptr += 4;
        memcpy(ptr, &batch.dataPoints[i].dataPointID, 4); ptr += 4;
    }

    // Prepare WebSocket packet
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;
    ws_pkt.payload = binary_data;
    ws_pkt.len = binary_size;


    ESP_LOGI(TAG_MAIN, "§§§§§§§§§§ JUST BEFORE SENDING - global_req set in ws_handler: %p", (void*)global_req);


      // Send the batch as binary data
    if (global_req != NULL && httpd_req_to_sockfd(global_req) >= 0) {
        ESP_LOGI(TAG_MAIN, "§§§Sending frame - global_req: %p", (void*)global_req);
        esp_err_t ret = httpd_ws_send_frame(global_req, &ws_pkt);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG_MAIN, "§§§Error sending frame: %s", esp_err_to_name(ret));
        }
    } else {
            ESP_LOGE(TAG_MAIN, "§§§global_req is NULL, skipping sending frame");
    }


    free(binary_data); // Free the allocated memory
    //if (ret != ESP_OK) {
    //      ESP_LOGE(TAG_MAIN, "Error sending frame: %s", esp_err_to_name(ret));
          //return ESP_FAIL;
    //}

    // Check if we've sent all batches
    if (batchCount > 50) {
        xTimerStop(xTimer, 0); // Stop the timer
        batchCount = 1; // Reset batch count for next time
        // Close the WebSocket connection
        // httpd_resp_send_done(global_req); // Uncomment if you want to close the connection
    }
}



void start_timer() {
    TimerHandle_t xTimer = xTimerCreate("Timer", pdMS_TO_TICKS(500), pdTRUE, 0, timerCallback);
    xTimerStart(xTimer, 0);
}



static esp_err_t ws_handler(httpd_req_t *req) {
    if (req != NULL) {
        ESP_LOGI(TAG_MAIN, "WebSocket Connection Started");
        global_req = req;
        start_timer();
        ESP_LOGI(TAG_MAIN, "global_req set in ws_handler: %p", (void*)global_req);
        
        // Return ESP_OK to indicate success
        return ESP_OK;
    } else {
        ESP_LOGE(TAG_MAIN, "WebSocket Connection Failed");
        // Return an error code to indicate failure
        return ESP_FAIL;
    }
    
    // Optional: Some compilers might still warn about no return at the end of the function.
    // You can optionally add a return here, but it's not strictly necessary because of the if/else logic above.
    // return ESP_OK; 
}


void close_handler() { //is not being used/implemented yet
    if (global_req != NULL) {
        global_req = NULL;
        ESP_LOGI(TAG_MAIN, "WebSocket connection closed and global_req invalidated");
    }
}



void start_webserver(void) {
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

        ESP_LOGI(TAG_MAIN, "Goodbye world 5555");

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
            break; 
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}
