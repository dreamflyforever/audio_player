#ifndef __PCM_RECORDER_H
#define __PCM_RECORDER_H

#include "pcm_trans.h"
#include "ff.h"

int pcm_recorder_start(char* player_path, uint32_t sample_rate, uint8_t channels);
int pcm_recorder_stop(void);

#endif
