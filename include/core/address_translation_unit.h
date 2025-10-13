#ifndef ADDR_TRANSLATION_UNIT
#define ADDR_TRANSLATION_UNIT

#include "core/cpu_internal.h"
#include "ids/register_ids.h"

typedef struct CPU CPU;
uint32_t address_translator(CPU *cpu, SegmentRegisterType seg, GeneralPurposeRegisterType offreg);

#endif
