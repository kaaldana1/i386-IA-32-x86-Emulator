#ifndef ALU_H
#define ALU_H
#include "cpu.h"
#include "ids/flag_registers.h"

typedef struct 
{
    uint32_t low, high;
    uint16_t flags_out;
} ALU_out;


int ALU(uint32_t op1, uint32_t op2, bool cin,
    size_t width,  Opclass opclass, ALU_out *out);

#endif
