// data_generation.h

#ifndef DATA_GENERATION_H
#define DATA_GENERATION_H

#include <stdint.h>
#include <stdlib.h>

#pragma pack(push, 1)
typedef struct {
    uint64_t timestamp;
    uint32_t channel1Value;
    uint32_t channel2Value;
    uint32_t dataPointID;
} DataPoint;

typedef struct {
    uint32_t batchID;
    DataPoint dataPoints[10]; // Adjust the size according to your requirements
} DataBatch;
#pragma pack(pop)

uint8_t* generate_data_batch(uint32_t batchCount, size_t *binary_size_out, int dataPointsPerBatch);


#endif // DATA_GENERATION_H
