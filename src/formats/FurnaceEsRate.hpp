// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <string_view>
namespace ngmv {
// Furnace reset writes initChanMax, then31, but its audio rate stays based
// on initChanMax. Older VGM dumps can leave this pair on PAGE=0x3f/W_END.
// Match the tagged reset signature only; never change generic ES5506 clocks.
class FurnaceEsRate {
    struct Pair {
        unsigned count=0,first=32;bool valid=true;
        void add(uint32_t value,uint64_t tick,bool full){
            if(tick||!full)valid=false;
            if(count==0){first=value;if(first<4||first>=31)valid=false;}
            else if(count!=1||value!=31)valid=false;
            ++count;
        }
    } low_,legacy_;
    uint32_t latch_=0;
    unsigned page_=0,reg_=16,mask_=0,serial_=0;
    bool serialStarted_=false;
public:
    void write(uint8_t address,uint8_t value,uint64_t tick){
        if(address>=64)return;
        unsigned reg=address/4,byte=address&3,shift=24-byte*8;
        if(byte==0){reg_=reg;mask_=0;}
        if(reg_!=reg)mask_=0;
        mask_|=1u<<byte;latch_=(latch_&~(255u<<shift))|(uint32_t(value)<<shift);
        if(byte!=3)return;
        bool full=mask_==15;mask_=0;
        if(reg==15){page_=latch_&127;return;}
        if(page_&64)return;
        if(!(page_&32)&&reg==11)low_.add(latch_,tick,full);
        if(page_==63&&reg==11&&!serialStarted_)legacy_.add(latch_,tick,full);
        if(!tick&&full&&(page_&32)){
            if(reg==10&&latch_==1){serial_|=1;serialStarted_=true;}
            if(reg==11&&latch_==17)serial_|=2;
            if(reg==12&&latch_==32)serial_|=4;
        }
        if(!tick&&full&&!(page_&32)&&reg==12&&latch_==8)serial_|=8;
    }
    unsigned voices(std::string_view creator)const{
        if(creator!="Furnace (chiptune tracker)"||serial_!=15)return 0;
        if(!legacy_.count&&low_.count==2&&low_.valid)return low_.first+1;
        if(!low_.count&&legacy_.count==2&&legacy_.valid)return legacy_.first+1;
        return 0;
    }
};
}
