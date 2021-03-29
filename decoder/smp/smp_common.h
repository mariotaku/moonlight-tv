#pragma once

#include "StarfishDirectMediaPlayer_C.h"

extern StarfishDirectMediaPlayer *playerctx;
extern DirectMediaVideoConfig videoConfig;
extern DirectMediaAudioConfig audioConfig;

extern unsigned long long pts;

void smp_player_create();
void smp_player_destroy();
void smp_player_open();
void smp_player_close();