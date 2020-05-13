#ifndef __PCM_PLAYER_H
#define __PCM_PLAYER_H

#include "pcm_trans.h"
#include "ff.h"

int pcm_player_start(char* player_path, uint32_t sample_rate, uint8_t channels, bool wait_finish);
int pcm_player_stop(void);

#endif
