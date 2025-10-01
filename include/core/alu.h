#ifndef ALU_H
#define ALU_H
#include "cpu.h"

typedef struct 
{
    uint32_t low, high;
    uint32_t cin;
    uint16_t flags_out;
} ALU_out;


int ALU(uint32_t op1, uint32_t op2, int cin,
    size_t width,  Opclass opclass, ALU_out *out);

#endif
