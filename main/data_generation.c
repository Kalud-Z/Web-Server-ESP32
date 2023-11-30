// data_generation.c
#include "data_generation.h" // Your custom header for data structures
#include "esp_log.h"         // For ESP logging functions
#include "esp_random.h"      // For random number generation
#include "esp_timer.h"       // For getting the current time
#include <stdlib.h>          // For malloc and NULL
#include <stdint.h>          // For fixed-width integer types
#include <string.h>



static const char *TAG = "DataGeneration";


uint8_t* generate_data_batch(uint32_t batchCount, size_t *binary_size_out, int dataPointsPerBatch) {    
    DataBatch batch;
    batch.batchID = batchCount;
    *binary_size_out = 4 + (dataPointsPerBatch * sizeof(DataPoint));
    uint8_t* binary_data = malloc(*binary_size_out);
    if (binary_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for binary data");
        return NULL;
    }

    uint8_t* ptr = binary_data;
    memcpy(ptr, &batch.batchID, 4); ptr += 4;
    uint64_t currentTime = esp_timer_get_time();

    for (int i = 0; i < dataPointsPerBatch; i++) {
        DataPoint point;
        point.timestamp = currentTime + (i * 1000);

        // Generate random values for channel1Value and channel2Value
        point.channel1Value = 10000000 + esp_random() % (16777215 - 10000000 + 1);
        point.channel2Value = 10000000 + esp_random() % (16777215 - 10000000 + 1);
        
        point.dataPointID = (batchCount - 1) * dataPointsPerBatch + i + 1;
        //ESP_LOGI(TAG, "Generated dataPointID: %u", point.dataPointID);
        //ESP_LOGI(TAG, "Generated dataPointID: %lu", (unsigned long)point.dataPointID);


        memcpy(ptr, &point, sizeof(DataPoint)); ptr += sizeof(DataPoint);
    }

    return binary_data;
}
