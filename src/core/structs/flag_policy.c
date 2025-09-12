#include "core/structs/flag_policy.h"

const Flag_policy ADD_flag_policy = 
{
    .write = ARITH_GROUP_WRITE,
    .clear = 0,
    .preserved = 0,
    .set = 0
};

const Flag_policy  SUB_flag_policy = 
{
    .write = (CF | OF | SF | ZF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0
};

const Flag_policy  CMP_flag_policy = 
{
    .write = (CF | OF | SF | ZF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0
};

const Flag_policy  NEG_flag_policy = 
{
    .write = (CF | OF | SF | ZF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0
};

const Flag_policy  ADC_flag_policy = 
{
    .write = (CF | OF | SF | ZF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0
};

const Flag_policy  SBB_flag_policy = 
{
    .write = (CF | OF | SF | ZF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0
};

const Flag_policy  SBB_flag_policy = 
{
    .write = (CF | OF | SF | ZF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0
};

const Flag_policy  SBB_flag_policy = 
{
    .write = (CF | OF | SF | ZF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0
};

const Flag_policy  SBB_flag_policy = 
{
    .write = (CF | OF | SF | ZF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0
};

const Flag_policy  SBB_flag_policy = 
{
    .write = (CF | OF | SF | ZF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0
};

const Flag_policy  SBB_flag_policy = 
{
    .write = (CF | OF | SF | ZF | PF | AF ),
    .clear = 0,
    .preserved = 0,
    .set = 0
};