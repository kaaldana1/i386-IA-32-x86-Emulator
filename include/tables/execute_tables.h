#ifndef EXECUTE_TABLES_H
#define EXECUTE_TABLES_H

#include "core/structs/instruction.h"  // use the canonical Instruction definition

typedef struct BUS BUS;
typedef struct CPU CPU;

/* Exported table */
extern int (*execution_handler_lut[256])(BUS *bus, CPU *cpu, Instruction *instr);

/* Call this at program start (safe to call multiple times). */
void init_execution_table(void);

#endif
