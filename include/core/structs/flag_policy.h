#ifndef FLAG_POLICY_H
#define FLAG_POLICY_H

#include <stdint.h>
#include "ids/opclass_list.h"

typedef struct 
{
    uint16_t write;
    uint16_t clear;
    uint16_t set;
    uint16_t preserved;
    uint16_t test;
} Flag_policy;

typedef enum 
{
    FP_ARITH_GRP_MASK = (OPC_ADD | OPC_SUB | OPC_CMP | OPC_NEG),
    FP_ARITH_2_GRP_MASK = (OPC_ADC | OPC_SBB),
    FP_INC_DEC_GRP_MASK = (OPC_INC | OPC_DEC),
    FP_LOGIC_GRP_MASK = (OPC_AND | OPC_XOR | OPC_OR | OPC_TEST),
    FP_SHIFT_GRP_MASK = (OPC_SHL | OPC_SHR | OPC_SAR),
    FP_ROTATE_GRP_MASK = (OPC_ROL | OPC_ROR),
} Flag_policy_mask;

extern const Flag_policy FP_ARITH_GROUP;
extern const Flag_policy FP_ARITH_2_GROUP;
extern const Flag_policy FP_INC_DEC_GROUP;
extern const Flag_policy FP_LOGIC_GRP;
extern const Flag_policy FP_SHIFT_GRP;
extern const Flag_policy FP_ROTATE_GRP;

#endif
