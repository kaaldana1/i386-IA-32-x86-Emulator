#include "tables/execute_tables.h"


extern int (*execution_handler_lut[256])(BUS *bus, CPU *cpu, Instruction *) = {
    #define X(name, _1, _2, _3, _4, _5) execute_##name,
        FOREACH_OPCODE(X)
    #undef X
};

