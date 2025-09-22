#ifndef INSTRUCTIONMETADATA_H
#define INSTRUCTIONMETADATA_H

#include <stdlib.h>
#include <stdbool.h>
#include "ids/opclass_list.h"

#define NO_IMMEDIATE      0
#define SINGLE_BYTE_IMM   1
#define TWO_BYTE_IMM      2
#define FOUR_BYTE_IMM     4
#define SIX_BYTE_IMM      6
#define PREFIX            7
#define REG_ONLY          8
#define IMM_TO_MEM        9

#define NO_MODRM          false
#define HAS_MODRM         true
#define PLACEHOLDER       0 
#define NO_OPERANDS       0

typedef enum
{
    REG_TO_REG,        
    REG_TO_IMM,        
    REG_TO_MEM,        
    MEM_TO_REG,        
    IMM_TO_REG,        
    GRP2_SHIFT,        
    GRP3_MISC ,        
    IMM_ONLY  ,        
    FPU       ,        
    RELATIVE  ,        
    PORT_IO   ,        
    GRP4_INCDEC,       
    GRP5_MISC  ,       
    BIDIR      ,       
    STR_OP     ,       
    TERNARY    ,       
    GRP1_IMM   ,       
    MEM_ONLY          
} Operand_type;

typedef struct
{
    bool has_modrm;
    size_t immediate_bytes;
    size_t operand_count;
    Operand_type operand_type;
    Opclass opclass;
    size_t width;
} InstructionMetadata;

#endif
