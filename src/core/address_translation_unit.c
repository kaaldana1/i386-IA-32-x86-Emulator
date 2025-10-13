#include "core/address_translation_unit.h"

uint32_t address_translator(CPU *cpu, SegmentRegisterType seg, GeneralPurposeRegisterType offreg)
{
    uint32_t offset = cpu->gen_purpose_registers[offreg].dword; 
    if (seg == SS)
        return cpu->segment_registers[seg].base + cpu->segment_registers[seg].limit - offset;
    else
        return cpu->segment_registers[seg].base + offset;
}
