#ifndef EXECUTOR_H
#define EXECUTOR_H
#include "core/structs/instruction.h"  // use the canonical Instruction definition
#include "core/cpu_internal.h"

typedef struct BUS BUS;
typedef struct CPU CPU;

#if defined(__GNUC__)
#  define WEAKREF __attribute__((weak))
#else
#  define WEAKREF
#endif

#define X(name, _op, _2, _3, _4, _5, _6, _7) \
    extern int execute_##name(BUS*, CPU*, Instruction*) WEAKREF; 
FOREACH_OPCODE(X)
#undef X
#endif
