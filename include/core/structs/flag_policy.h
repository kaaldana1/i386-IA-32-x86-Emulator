#ifndef FLAG_POLICY_H
#define FLAG_POLICY_H

#include <stdint.h>
#include "ids/flag_registers.h"

#define ARITH_GROUP_WRITE (CF | OF | SF | ZF | PF | AF )

typedef struct 
{
    uint8_t write;
    uint8_t clear;
    uint8_t set;
    uint8_t preserved;
} Flag_policy;


#endif