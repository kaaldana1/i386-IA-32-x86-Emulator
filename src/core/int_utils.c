#include "core/int_utils.h"


uint16_t get_SegmentRegister_index(const SegmentRegister *reg) 
{
    return reg->selector >> 3;
}

uint64_t get_descriptor(BUS *bus, GDTR *gdtr, uint16_t index) 
{
    if ((index * GDT_DESC_SIZE) + 7 > gdtr->size) { return 0; }
    uint32_t low, high;
    bus_read(bus, &low, gdtr->base + index * GDT_DESC_SIZE, 32);
    bus_read(bus, &high, gdtr->base + (index * GDT_DESC_SIZE) + 4, 32);
    return ((uint64_t)low) | ((uint64_t)high << 32);
}

uint32_t get_base(uint64_t descriptor)
{
    return ( (((uint32_t) descriptor >> 16) & 0xFFFFFFu) | // captures base_low and base_mid (byte 2-4)
        ((uint32_t)(descriptor >> 56) << 24)); // captures high (byte 7)
}

uint32_t get_limit(uint64_t descriptor) 
{
    return ( ((uint32_t)descriptor & 0xFFFF) | 
    (((uint32_t)(descriptor >> 48) & 0x0F) << 16));
}

uint16_t get_flag(uint64_t descriptor) 
{
    return( (uint16_t)(descriptor >> 40) & 0xF0FF);
}

uint32_t address_translator(CPU *cpu, SegmentRegisterType seg, GeneralPurposeRegisterType offreg)
{
    uint32_t offset = cpu->gen_purpose_registers[offreg].dword; 
    if (seg == SS)
        return cpu->segment_registers[seg].base + cpu->segment_registers[seg].limit + offset;
    else
        return cpu->segment_registers[seg].base + offset;
}
