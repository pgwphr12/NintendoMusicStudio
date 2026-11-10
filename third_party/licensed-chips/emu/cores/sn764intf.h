// Own functional configuration ABI for the selected SN core.
#pragma once
#include "../../compat.h"
typedef struct {
 DEV_GEN_CFG _genCfg;
 UINT16 noiseTaps;
 UINT8 shiftRegWidth,negate,clkDiv,ncrPSG,segaPSG,stereo;
 void*t6w28_tone;
} SN76496_CFG;
#define SN76496_W_REG 0
#define SN76496_W_GGST 1
