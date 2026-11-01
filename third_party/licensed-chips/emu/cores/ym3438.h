/* Own declarations of the public Nuked OPN2 C functions. */
#pragma once
#include "../../compat.h"
void* nukedopn2_init(UINT32,UINT32);void nukedopn2_shutdown(void*);void nukedopn2_reset_chip(void*);
void nukedopn2_write(void*,UINT8,UINT8);void nukedopn2_update(void*,UINT32,DEV_SMPL**);
void nukedopn2_set_mute_mask(void*,UINT32);void nukedopn2_set_options(void*,UINT32);
