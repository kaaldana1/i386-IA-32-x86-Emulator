#ifndef ADDR_TRANSLATION_UNIT
#define ADDR_TRANSLATION_UNIT

#include "core/cpu_internal.h"
#include "ids/register_ids.h"

typedef struct CPU CPU;
uint32_t address_translator(CPU *cpu, SegmentRegisterType seg, GeneralPurposeRegisterType offreg);

uint16_t get_SegmentRegister_index(const SegmentRegister *reg);
uint32_t get_base(uint64_t descriptor);
uint32_t get_limit(uint64_t descriptor);
uint16_t get_flag(uint64_t descriptor); 
uint64_t get_descriptor(BUS *bus, GDTR *gdtr, uint16_t index);


#endif
