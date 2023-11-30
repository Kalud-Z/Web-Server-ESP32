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

uint8_t* generate_data_batch(uint32_t batchCount, size_t *binary_size_out, int dataPointsPerBatch,  int numberOfChannels) {
    *binary_size_out = 4 + (dataPointsPerBatch * (sizeof(uint64_t) + (numberOfChannels * sizeof(uint32_t)) + sizeof(uint32_t)));
    uint8_t* binary_data = malloc(*binary_size_out);
    if (binary_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for binary data");
        return NULL;
    }

    uint8_t* ptr = binary_data;
    // Write batch ID at the start of the binary data
    memcpy(ptr, &batchCount, 4); ptr += 4;

    // Constants for the sine wave generation
    const uint32_t min = 10000000;
    const uint32_t max = 16777215;
    const int totalPointsInPeriod = 1000; // Points in one period of the sine wave

    static uint32_t dataPointId = 0; // Static variable to keep track across function calls
    static uint32_t totalDataPointsGenerated = 0; // Static variable to keep track across function calls

    for (int i = 0; i < dataPointsPerBatch; i++) {
        totalDataPointsGenerated++; // Increment total data points generated

        // Write timestamp
        uint64_t timestamp = esp_timer_get_time();
        memcpy(ptr, &timestamp, sizeof(timestamp)); ptr += sizeof(timestamp);

        // Write channel values
        for (int channel = 0; channel < numberOfChannels; channel++) {
            // Calculate the sine wave values for each channel
            double angle = (2 * M_PI * (totalDataPointsGenerated % totalPointsInPeriod)) / totalPointsInPeriod;
            uint32_t value = generate_sine_data(angle, min, max);
            memcpy(ptr, &value, sizeof(value)); ptr += sizeof(value);
        }

        // Write data point ID
        memcpy(ptr, &dataPointId, sizeof(dataPointId)); ptr += sizeof(dataPointId);
        dataPointId++;
    }

    return binary_data;
}
