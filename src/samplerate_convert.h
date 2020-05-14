#ifndef __SAMPLERATE_CONVERT_H
#define __SAMPLERATE_CONVERT_H

#include "FreeRTOS.h"
#include "samplerate.h"
#include "ring_buffer.h"

#define SAMPLERATE_CONVERT_INPUT_SIZE   1024
#define SAMPLERATE_CONVERT_OUTPUT_SIZE  (SAMPLERATE_CONVERT_INPUT_SIZE * 2)

typedef struct{
    SRC_STATE *src_state;
	SRC_DATA src_data;
    float input_buffer[SAMPLERATE_CONVERT_INPUT_SIZE];
    float output_buffer[SAMPLERATE_CONVERT_OUTPUT_SIZE];
    uint8_t output_bytes[SAMPLERATE_CONVERT_OUTPUT_SIZE*2];

    ring_buffer_t* ring_buffer;
    
} samplerate_convert_t;

samplerate_convert_t* samplerate_convert_alloc(ring_buffer_t* ring_buffer);
void samplerate_convert_dealloc(samplerate_convert_t** pp_samplerate);
int samplerate_convert_process(samplerate_convert_t* samplerate, double src_ratio, int channels, uint8_t* buffer, int len);

#endif
