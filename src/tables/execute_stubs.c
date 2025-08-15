
#include "tables/execute_tables.h"
 __attribute__((weak))

#define X(name, _1, _2, _3, _4, _5) __attribute__((weak)) int execute_##name(BUS *bus, CPU *cpu, Instruction *instr) { (void)bus; (void)cpu; (void)instr; return 1; }
FOREACH_OPCODE(X)
#undef X
