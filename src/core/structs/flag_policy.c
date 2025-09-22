#include "core/structs/FlagPolicy.h"
#include "ids/register_ids.h"

// Categories of OPCLASS that share flag update logic:
/*
Instruction                               OF  SF      ZF  AF  PF  CF
                    ===================================================
GROUP 1             AAA                   ──  ──      ──  TM  ──  M
                    AAS                   ──  ──      ──  TM  ──  M
                    ===================================================
GROUP 2             AAD                   ──  M   M   ──  M   ── 
                    AAM                   ──  M   M   ──  M   ── 
                    ===================================================
GROUP 3             DAA                   ──  M   M   TM  M   TM 
                    DAS                   ──  M   M   TM  M   TM 
                    ===================================================
GROUP 4             ADC                   M   M   M   M   M   TM 
                    SBB                   M   M   M   M   M   TM 
                    ===================================================
FP_ARITH_GRP        ADD                   M   M   M   M   M   M 
                    SUB                   M   M   M   M   M   M 
                    CMP                   M   M   M   M   M   M 
                   CMPS                   M   M   M   M   M   M 
                   SCAS                   M   M   M   M   M   M 
                    NEG                   M   M   M   M   M   M 
                    ===================================================
GROUP 5             DEC                   M   M   M   M   M  
                    INC                   M   M   M   M   M  
                    ===================================================
GROUP 6             IMUL                  M  ──  ──  ──  ──   M 
                    MUL                   M  ──  ──  ──  ──   M 
                    ===================================================
GROUP 7            RCL/RCR 1              M  TM
                    ===================================================
GROUP 8            RCL/RCR count          ── TM
                    ===================================================
GROUP 9            ROL/ROR1               M   M
                    ===================================================
GROUP 10            ROL/ROR count         ──  M
                    ===================================================
GROUP 11            SAL/SAR/SHL/SHR 1     M   M   M  ──   M   M
                    ===================================================
GROUP 12            SAL/SAR/SHL/SHR count ──  M   M  ──   M   M
                   SHLD/SHRD              ──  M   M  ──   M   M
                    ===================================================
GROUP 13            BSF/BSegmentRegister               ── ──   M  ──  ──  ──
                    ===================================================
GROUP 14            BT/BTS/BTR/BTC        ── ──  ──  ──  ──   M
                    ===================================================
GROUP 15            AND                   0   M   M  ──   M   0
                   OR                     0   M   M  ──   M   0
                   TEST                   0   M   M  ──   M   0
                   XOR                    0   M   M  ──   M   0
*/

    /*FP_ARITH_GRP_MASK = (OPC_ADD | OPC_SUB | OPC_CMP | OPC_NEG),
FP_ARITH_2_GRP_MASK = (OPC_ADC | OPC_SBB),

    FP_INC_DEC_GRP_MASK = (OPC_INC | OPC_DEC),

    FP_LOGIC_GRP_MASK = (OPC_AND | OPC_XOR | OPC_OR | OPC_TEST),

    FP_SHIFT_GRP_MASK = (OPC_SHL | OPC_SHR | OPC_SAR),

    FP_ROTATE_GRP_MASK = (OPC_ROL | OPC_ROR),
    */

const FlagPolicy  FP_ARITH_GROUP = 
{
    .write =  (CF | OF | SF | ZF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0,
    .test = 0
};

const FlagPolicy FP_ARITH_2_GROUP = 
{
    .write =  (CF | OF | SF | ZF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0,
    .test = PF
};

const FlagPolicy FP_INC_DEC_GROUP = 
{
    .write =  (OF | SF | ZF | AF | PF ),
    .clear = 0,
    .preserved = CF,
    .set = 0,
    .test = 0
};

const FlagPolicy FP_LOGIC_GRP = 
{
    .write = (SF | ZF | PF ),
    .clear = OF,
    .preserved = 0,
    .set = 0,
    .test = 0
};

const FlagPolicy FP_SHIFT_GRP = 
{
    .write =  (CF | OF | SF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0,
    .test = 0
};

const FlagPolicy FP_ROTATE_GRP = 
{
    .write =  ( SF | OF ),
    .clear = 0,
    .preserved = 0,
    .set = 0,
    .test = 0
};

 