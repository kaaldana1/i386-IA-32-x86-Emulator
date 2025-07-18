#ifndef EXECUTE_H
#define EXECUTE_H

#include "decoder.h"
#include "memory.h"

void print_registers();

#define X(name, _1, _2, _3, _4) int execute_##name(Instruction *decoded_instr);
    FOREACH_OPCODE(X)
#undef X

// instead of switch statements, a table of function pointers, indexed by Opcode_ID
static int (*execution_handler_lut[256])(Instruction *) = {
    #define X(name, _1, _2, _3, _4) execute_##name,
        FOREACH_OPCODE(X)
    #undef X
};

#endif