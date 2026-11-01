/* SPDX-License-Identifier: GPL-2.0-or-later
 * Independently authored minimal functional C ABI for the selected licensed
 * chip cores. This is not libvgm's framework/header implementation. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t UINT8;typedef int8_t INT8;
typedef uint16_t UINT16;typedef int16_t INT16;
typedef uint32_t UINT32;typedef int32_t INT32;
typedef uint64_t UINT64;typedef int64_t INT64;
typedef INT32 DEV_SMPL;
typedef struct {void *chipInf;} DEV_DATA;
typedef struct {UINT32 emuCore;UINT8 srMode,flags;UINT32 clock,smplRate;} DEV_GEN_CFG;
typedef struct DEV_DEF DEV_DEF;
typedef struct DEV_DECL DEV_DECL;
typedef struct DEV_INFO DEV_INFO;
typedef struct {unsigned unused;} DEVLINK_IDS;
typedef void (*DEVCB_SRATE_CHG)(void*,UINT32);
typedef void (*DEVCB_LOG)(void*,void*,UINT8,const char*);
typedef struct {UINT8 funcType,rwType;UINT16 user;void*funcPtr;} DEVDEF_RWFUNC;
struct DEV_DEF {
 const char *name,*author;UINT32 coreID;
 UINT8(*Start)(const DEV_GEN_CFG*,DEV_INFO*);void(*Stop)(void*);void(*Reset)(void*);
 void(*Update)(void*,UINT32,DEV_SMPL**);void(*SetOptionBits)(void*,UINT32);
 void(*SetMuteMask)(void*,UINT32);void(*SetPanning)(void*,const INT16*);
 void(*SetSRateChgCB)(void*,DEVCB_SRATE_CHG,void*);
 void(*SetLogCB)(void*,DEVCB_LOG,void*);UINT8(*LinkDevice)(void*,UINT8,const DEV_INFO*);
 const DEVDEF_RWFUNC *rwFuncs;
};
struct DEV_DECL {UINT8 deviceID;const char*(*name)(const DEV_GEN_CFG*);
 UINT16(*channelCount)(const DEV_GEN_CFG*);const char**(*channelNames)(const DEV_GEN_CFG*);
 const DEVLINK_IDS*(*linkDevIDs)(const DEV_GEN_CFG*);const DEV_DEF*cores[4];};
struct DEV_INFO {DEV_DATA*dataPtr;UINT32 sampleRate;const DEV_DEF*devDef;};
typedef struct {DEVCB_LOG cb;void *user,*chip;} DEV_LOGGER;
#define INLINE static __inline
INLINE void dev_logger_set(DEV_LOGGER*l,void*p,DEVCB_LOG cb,void*u){l->chip=p;l->cb=cb;l->user=u;}
INLINE void emu_logf(DEV_LOGGER*l,UINT8 level,const char*format,...){
 if(l->cb){char text[1024];va_list args;va_start(args,format);vsnprintf(text,sizeof(text),format,args);va_end(args);l->cb(l->user,l->chip,level,text);}}
INLINE void own_chip_init(DEV_INFO*i,DEV_DATA*p,UINT32 rate,const DEV_DEF*d){i->dataPtr=p;i->sampleRate=rate;i->devDef=d;}
#define INIT_DEVINF(i,p,r,d) own_chip_init(i,p,r,d)
#define SRATE_CUSTOM_HIGHEST(mode,rate,wanted) do{if((mode)==1 || ((mode)==2 && (wanted)>(rate)))(rate)=(wanted);}while(0)
enum {RWF_WRITE=0,RWF_READ=1,RWF_REGISTER=0,RWF_MEMORY=16,RWF_CLOCK=128,RWF_SRATE=130,RWF_CHN_MUTE=144};
enum {DEVRW_VALUE=0,DEVRW_ALL=1,DEVRW_A8D8=17,DEVRW_BLOCK=128,DEVRW_MEMSIZE=129};
enum {DEVLOG_ERROR=1,DEVLOG_WARN=2,DEVLOG_INFO=3,DEVLOG_DEBUG=4};
enum {FCC_MAME=1,DEVID_K051649=1,DEVID_MSM6295=2};
