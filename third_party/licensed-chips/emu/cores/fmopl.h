/* Own functional declarations; only the licensed OPL2 implementation is built. */
#pragma once
#include "../../compat.h"
#define BUILD_YM3812 1
#define BUILD_YM3526 0
#define BUILD_Y8950 0
typedef void(*OPL_TIMERHANDLER)(void*,UINT8,UINT32);
typedef void(*OPL_IRQHANDLER)(void*,UINT8);
typedef void(*OPL_UPDATEHANDLER)(void*);
typedef void(*OPL_PORTHANDLER_W)(void*,UINT8);
typedef UINT8(*OPL_PORTHANDLER_R)(void*);
void* ym3812_init(UINT32,UINT32);void ym3812_shutdown(void*);void ym3812_reset_chip(void*);
void ym3812_write(void*,UINT8,UINT8);void ym3812_update_one(void*,UINT32,DEV_SMPL**);
void opl_set_mute_mask(void*,UINT32);
