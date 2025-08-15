#include "core/cpu.h"
#define MAX_INSTR_LENGTH 16


static inline uint16_t get_SR_index(const SR *reg) 
{
    return reg->selector >> 3;
}

static inline uint64_t get_descriptor(BUS *bus, GDTR *gdtr, uint16_t index) 
{
    if ((index * GDT_DESC_SIZE) + 7 > gdtr->size) { return 0; }
    uint32_t low, high;
    bus_read(bus, &low, gdtr->base + index * GDT_DESC_SIZE);
    bus_read(bus, &high, gdtr->base + (index * GDT_DESC_SIZE) + 4);
    return ((uint64_t)low) | ((uint64_t)high << 32);
}

static inline uint32_t get_base(uint64_t descriptor)
{
    return ( (((uint32_t) descriptor >> 16) & 0xFFFFFFu) | // captures base_low and base_mid (byte 2-4)
        ((uint32_t)(descriptor >> 56) << 24)); // captures high (byte 7)
}

static inline uint32_t get_limit(uint64_t descriptor) 
{
    return ( ((uint32_t)descriptor & 0xFFFF) | 
    (((uint32_t)(descriptor >> 48) & 0x0F) << 16));
}

static inline uint16_t get_flag(uint64_t descriptor) 
{
    return( (uint16_t)(descriptor >> 40) & 0xF0FF);
}

int set_SR_cache(BUS *bus, CPU *cpu, SR_type type) 
{
    uint16_t index = get_SR_index(&cpu->segment_registers[type]); // gets index bits from selector
    uint64_t desc = get_descriptor(bus, &cpu->gdtr, index); 

    cpu->segment_registers[type].base = get_base(desc);
    cpu->segment_registers[type].limit = get_limit(desc);
    cpu->segment_registers[type].flag = get_flag(desc);
    return 1;
}

CPU *create_cpu(void) 
{
    CPU *cpu = calloc(1, sizeof(CPU));
    return cpu;
}

int cpu_protected_mode_reset(BUS *bus, CPU *cpu, uint32_t start_eip, uint32_t start_esp, 
                            uint32_t gdtr_base, uint32_t gdtr_size)
{
    cpu->gdtr.base = gdtr_base;
    cpu->gdtr.size = gdtr_size;

    cpu->gen_purpose_registers[EIP].dword = start_eip;
    cpu->gen_purpose_registers[ESP].dword = start_esp;

    cpu->segment_registers[CS].selector = 0x0018;
    cpu->segment_registers[DS].selector = 0x0023;
    cpu->segment_registers[SS].selector = 0x0023;

    set_SR_cache(bus, cpu, CS);
    set_SR_cache(bus, cpu, DS);
    set_SR_cache(bus, cpu, SS);

    return 1;
}

static void to_byte_array(uint32_t *dword_arr, uint8_t *byte_arr) 
{
    memset(byte_arr, 0x00, MAX_INSTR_LENGTH );
    int byte_index = 0;
    int dword_index = 0;
    int shift = 0;
    bool first_loop = true;
    while(byte_index < MAX_INSTR_LENGTH) 
    {
        if (shift == 32) 
        { 
            shift = 0; 
            continue; 
        } 
        else 
        {
             byte_arr[byte_index] = (uint8_t)(dword_arr[dword_index] >> shift); 
             shift += 8; 
        }
        (((byte_index + 1) % 4) == 0 && !first_loop) ? (dword_index++, byte_index++) : byte_index++;
        first_loop = false;
    }
}

static int prefetch (BUS *bus, uint32_t *queue, uint32_t addr) 
{
    memset(queue, 0, (MAX_INSTR_LENGTH / 4) * sizeof(uint32_t));
    for(size_t i = 0; i < MAX_INSTR_LENGTH / 4; i++)  // reads four dwords (16 bytes)
    {
        bus_read(bus, (queue + i), addr + i*4);
    }
    return 1;
}

int interpreter(CPU *cpu, BUS *bus) 
{
    uint32_t start_addr;
    uint32_t CS_base = cpu->segment_registers[CS].base;
    uint32_t CS_limit = cpu->segment_registers[CS].limit;

    uint32_t instr_queue[MAX_INSTR_LENGTH / 4];
    uint8_t byte_instr_queue[MAX_INSTR_LENGTH];
    while(cpu->gen_purpose_registers[EIP].dword < (CS_base + CS_limit) && !cpu->halt) 
    {
        start_addr = cpu->gen_purpose_registers[EIP].dword;
        prefetch(bus, instr_queue, start_addr);
        to_byte_array(instr_queue, byte_instr_queue);
        Instruction *decoded_instruction = decoder(byte_instr_queue);
        cpu->gen_purpose_registers[EIP].dword += decoded_instruction->total_length;

        if ((*execution_handler_lut[decoded_instruction->opcode_id])(bus, cpu, decoded_instruction))
        {
            printf("\nExecution complete\n");
        }
    }
    return 1;
}

