#ifndef CPU_H
#define CPU_H

#include "ids/register_ids.h"
#include "decoder.h"
#include "executor.h"


typedef struct CPU CPU;
CPU *create_cpu(void);
int cpu_protected_mode_reset(BUS *bus, CPU *cpu, 
                            uint32_t start_eip, 
                            uint32_t start_esp, 
                            uint32_t gdtr_base,
                            uint32_t gdtr_size);

int interpreter(CPU *cpu, BUS *bus);

#endif
