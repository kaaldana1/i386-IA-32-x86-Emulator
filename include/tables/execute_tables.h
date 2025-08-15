#ifndef EXECUTE_TABLES_H
#define EXECUTE_TABLES_H

#include "core/structs/instruction.h"

typedef struct BUS BUS;
typedef struct CPU CPU;
#define X(name, _1, _2, _3, _4, _5) int execute_##name(BUS *bus, CPU *cpu, Instruction *decoded_instr);
    FOREACH_OPCODE(X)
#undef X

extern int (*execution_handler_lut[256])(BUS *bus, CPU *cpu, Instruction *decoded_instr);

#endif
