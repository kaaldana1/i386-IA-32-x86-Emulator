#include "core/cpu.h"
#include "core/int_utils.h"
#include "ui/display_api.h"
#include "core/time_sim.h"
#include "core/interrupt/interrupt_controller.h"
#include "core/interrupt/interrupt_handlers.h"

#define MAX_INSTR_LENGTH 16


int set_SegmentRegister_cache(BUS *bus, CPU *cpu, SegmentRegisterType type) 
{
    uint16_t index = get_SegmentRegister_index(&cpu->segment_registers[type]); // gets index bits from selector
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

int cpu_protected_mode_reset(BUS *bus, CPU *cpu, uint32_t gdtr_base, uint32_t gdtr_size)
{
    cpu->gdtr.base = gdtr_base;
    cpu->gdtr.size = gdtr_size;

    cpu->segment_registers[CS].selector = 0x13;
    cpu->segment_registers[DS].selector = 0x1B;
    cpu->segment_registers[SS].selector = 0x23;

    // changed: EIP and ESP are purely offsets
    cpu->gen_purpose_registers[EIP].dword = 0;
    cpu->gen_purpose_registers[ESP].dword = 0;

    set_SegmentRegister_cache(bus, cpu, CS);
    set_SegmentRegister_cache(bus, cpu, DS);
    set_SegmentRegister_cache(bus, cpu, SS);

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
        bus_read(bus, (queue + i), addr + i*4, 32);
    }
    return 1;
}

static void execute_pending_interrupts()
{
    for (int i = 0; i < IRQ_COUNT; i++)
    {
        if (irc.enabled[i] && irc.pending[i])
            (*irq_handler_table[i])();
    }
}

int interpreter(CPU *cpu, BUS *bus) 
{
    uint32_t start_addr;
    uint32_t CS_base = cpu->segment_registers[CS].base;
    uint32_t CS_limit = cpu->segment_registers[CS].limit;

    uint32_t instr_queue[MAX_INSTR_LENGTH / 4];
    uint8_t byte_instr_queue[MAX_INSTR_LENGTH];
    while(address_translator(cpu, CS, EIP) < (CS_base + CS_limit) && !cpu->halt) 
    {
        execute_pending_interrupts();
        start_addr = address_translator(cpu, CS, EIP);
        prefetch(bus, instr_queue, start_addr);
        to_byte_array(instr_queue, byte_instr_queue);
        Instruction *decoded_instruction = decoder(byte_instr_queue);

        cpu->gen_purpose_registers[EIP].dword += decoded_instruction->total_length;

        #ifdef NCURSES_ON
        machine_state.ui_callbacks.ui_copy_instr_after_decode(decoded_instruction);
        #endif

        (*execution_handler_lut[decoded_instruction->opcode[0]])(bus, cpu, decoded_instruction);

        time++;

        
        #ifdef NCURSES_ON
        machine_state.ui_callbacks.ui_copy_cpu_after_execute(cpu);
        machine_state.ui_callbacks.ui_reset_stack_after_execute();
        machine_state.ui_callbacks.ui_flush_ui();
        #endif
    }
    return 1;
}

