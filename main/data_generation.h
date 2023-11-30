// data_generation.h

#ifndef DATA_GENERATION_H
#define DATA_GENERATION_H

#include <stdint.h>
#include <stdlib.h>

#pragma pack(push, 1)

// Flexible array member for channel values
typedef struct {
    uint64_t timestamp;
    uint32_t dataPointID;
    uint32_t channelValues[]; // Flexible array member for channel values
} DataPoint;

// DataBatch structure to hold a batch of data points
typedef struct {
    uint32_t batchID;
    // Since DataPoint now uses a flexible array member, we can't have an array of DataPoint.
    // Instead, we'll manage the data batch as a contiguous block of memory in the C file.
} DataBatch;

#pragma pack(pop)

// Function prototype to generate a batch of data points
uint8_t* generate_data_batch(uint32_t batchCount, size_t *binary_size_out, int dataPointsPerBatch, int numberOfChannels);

#endif // DATA_GENERATION_H
