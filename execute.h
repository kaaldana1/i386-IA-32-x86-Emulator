#ifndef EXECUTE_H
#define EXECUTE_H

#include "decoder.h"

extern bool halt_flag;

void print_registers();

#define X(name, _1, _2, _3, _4, _5) int execute_##name(Instruction *decoded_instr);
    FOREACH_OPCODE(X)
#undef X

// instead of switch statements, a table of function pointers, indexed by Opcode_ID
static int (*execution_handler_lut[256])(Instruction *) = {
    #define X(name, _1, _2, _3, _4, _5) execute_##name,
        FOREACH_OPCODE(X)
    #undef X
};

#endif