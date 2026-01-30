#include "ids/return_code_list.h"
#include "tables/execute_tables.h"
#include "core/executor.h"
#include "ids/opcode_list.h"

// execution stubs
static int noop(BUS *bus, CPU *cpu, Instruction *instr) 
{
    (void)bus; (void)cpu; (void)instr;
    return UNIMPLEMENTED_INSTRUCTION;
}

// start with all entries pointing to the default
int (*execution_handler_lut[256])(BUS*, CPU*, Instruction*);

static void init_execution_handler_lut(void) 
{
    for (int i = 0; i < 256; ++i)
        execution_handler_lut[i] = noop;
}

#define X(name, _op_byte, _2, _3, _4, _5, _6, _7) \
__attribute__((weak)) \
int execute_##name(BUS *bus, CPU *cpu, Instruction *instr) { \
    (void)bus; (void)cpu; (void)instr; \
    return UNIMPLEMENTED_INSTRUCTION; \
}
FOREACH_OPCODE(X)
#undef X

static void patch_execution_table(void) {
#define X(name, op_byte, _2, _3, _4, _5, _6, _7) \
    execution_handler_lut[(unsigned)(op_byte)] = execute_##name;
    FOREACH_OPCODE(X)
#undef X
}

void init_execution_table(void) 
{
    init_execution_handler_lut();
    patch_execution_table();
}



