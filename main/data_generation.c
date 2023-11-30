// data_generation.c
#include "data_generation.h" // Your custom header for data structures
#include "esp_log.h"         // For ESP logging functions
#include "esp_random.h"      // For random number generation
#include "esp_timer.h"       // For getting the current time
#include <stdlib.h>          // For malloc and NULL
#include <stdint.h>          // For fixed-width integer types
#include <string.h>
#include <math.h>            // Include the math library for sin function


static const char *TAG = "DataGeneration";



// Helper function to generate sine wave data
static uint32_t generate_sine_data(double angle, uint32_t min, uint32_t max) {
    double sine_value = sin(angle);
    return (uint32_t)(((sine_value + 1) / 2) * (max - min) + min);
}

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

    // Constants for the sine wave generation
    const uint32_t min = 10000000;
    const uint32_t max = 16777215;
    const int totalPointsInPeriod = 1000; // Points in one period of the sine wave

    static uint32_t dataPointId = 0; // Static variable to keep track across function calls
    static uint32_t totalDataPointsGenerated = 0; // Static variable to keep track across function calls

    for (int i = 0; i < dataPointsPerBatch; i++) {
        totalDataPointsGenerated++; // Increment total data points generated

        DataPoint point;
        point.timestamp = esp_timer_get_time();

        // Calculate the sine wave values
        double angle = (2 * M_PI * (totalDataPointsGenerated % totalPointsInPeriod)) / totalPointsInPeriod;
        point.channel1Value = generate_sine_data(angle, min, max);
        point.channel2Value = generate_sine_data(angle, min, max);

        point.dataPointID = dataPointId++;

        memcpy(ptr, &point, sizeof(DataPoint)); ptr += sizeof(DataPoint);
    }

    return binary_data;
}


