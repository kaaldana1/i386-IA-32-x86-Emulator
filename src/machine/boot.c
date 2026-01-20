#include <ncurses.h>
#include "machine/boot.h"
#include "hardware_sim/memmap.h"
#include "hardware_sim/devices_internal.h"
#include "ui/display_api.h"
#include "core/interrupt/interrupt_controller.h"
#include "core/interrupt/interrupt_handlers.h"
#include "core/clock.h"
#include "ids/return_code_list.h"

typedef struct 
{
    CPU *cpu;
    BUS *bus;
    RAMDev *ram;
    VGADev *vga;
    KeyboardDev *keyboard;
    Clock *clock;
} Machine;

static Machine *boot_sequence(Program *p);
static int create_addr_space(RAMDev *ram, BUS *bus, CONSOLEDev *c, VGADev *vga, KeyboardDev *keyboard);
static void destroy_machine(Machine *machine);

int start(Program *p) 
{
    Machine *machine = boot_sequence(p);
    if (machine == NULL) { return MACHINE_FAILURE; }
    int ch = 0;
    int result = 0;
    while (ch != 'q') 
    {
        result = interpreter(machine->cpu, machine->bus, machine->clock);
        if (!(result == END_OF_PROGRAM || result == EXECUTE_SUCCESS)) { destroy_machine(machine); return result; }
        ch = getch();
        if (ch != ERR) 
        {
            KeyboardDev *k = machine->keyboard;
            k->enqueue(k->keyboard_buffer, (uint8_t)ch, (size_t *)&k->keystrokes_in_queue, sizeof(k->keyboard_buffer));
            k->interrupter.interrupt_line(k->interrupter.irq_num);
        }

        if (read_clock(machine->clock) % machine->vga->refresh_rate == 0)
            machine->vga->interrupter.interrupt_line(machine->vga->interrupter.irq_num);

        if (ui_on)
            machine_state.ui_callbacks.ui_flush_ui();
    }
    destroy_machine(machine);
    return MACHINE_TURN_OFF;
}

static Machine *boot_sequence(Program *p) 
{
    //TODO: Define addresses here, parameterize the addresses. Dont let functions hide them
    RAMDev *ram = init_ram(); // 16 kib array
    CONSOLEDev *c = init_console();
    VGADev *vga = init_vga();
    KeyboardDev *keyboard = init_keyboard();
    Clock *clock = init_clock();

    if (ram == NULL || c == NULL || vga == NULL || keyboard == NULL || clock == NULL) { return NULL; }

    // BUS has an address table, an entry in the table expects:
        /*
            uint32_t base;
            uint32_t size;
            read_func read;
            write_func write;
            void *device;
        */

    BUS *bus = create_bus(MAX_DEVICES);
    if (bus == NULL) { return NULL; }
    
    init_interrupt_handler(bus, vga, keyboard);
    init_interrupt_controller(keyboard, vga);

    // Register devices into the bus:
        // Ram starts at 0x0, size is 16kb
        // Console port is at address 0x4000
    if (!create_addr_space(ram, bus, c, vga, keyboard)) { return NULL; }
    
    // Creates GDT at GDT base
        /* Descriptors:
            (0x0000, 0x07FF, KERNEL_CODE_RX
            (0x0800, 0x07FF, USER_CODE_RX 
            (0x1000, 0x27FF, USER_DATA_RW  
            (0x1800, 0x27FF, STACK
        */
    if (!create_gdt(bus, GDT_BASE_ADDR)) { return NULL; }

    CPU *cpu = create_cpu();
    if (cpu == NULL) { return NULL; }

    // Sets GDTR base pointer in CPU
    // Sets selector bits in Segment Register:
        /*
    cpu->segment_registers[CS].selector = 0x1 -> Indexes into User Code descriptor
    cpu->segment_registers[DS].selector = 0x1 -> Indexes into User Data desciptor
    cpu->segment_registers[SS].selector = 0x2 -> Indexes into User Stack desciptor
        */
    cpu_protected_mode_reset(bus, cpu, GDT_BASE_ADDR, GDT_SIZE);
    init_execution_table();
    if (load_program(bus, p, USER_CODE_BASE_ADDR) == 0) { return NULL; }

    Machine *m = (Machine *)calloc(1, sizeof(Machine));
    *m = (Machine){.bus = bus, .cpu = cpu, .ram = ram, .keyboard = keyboard, .vga = vga, .clock = clock};
    if (m == NULL) { return NULL; }

    return m;
}

static int create_addr_space(RAMDev *ram,  BUS *bus, CONSOLEDev *c, VGADev *vga, KeyboardDev *keyboard) 
{
    if (!bus_register(bus, RAM_BASE_ADDR, RAM_SIZE, ram_read, ram_write, ram)) { return 0; }
    if (!bus_register(bus, CONSOLE_BASE_ADDR, CONSOLE_SIZE, console_read_stub, console_write, c)) { return 0; }
    if (!bus_register(bus, VGA_BASE_ADDR, VGA_SIZE, vram_read_stub, vram_write, vga)) { return 0; }
    if (!bus_register(bus, KEYBOARD_BASE_ADDR, KEYBOARD_SIZE, keyboard_read, keyboard_write, keyboard)) { return 0; }
    return 1;
}

static void destroy_machine(Machine *machine) 
{
    if (machine->bus != NULL) { bus_destroy(machine->bus); }
    if (machine->cpu != NULL) { free(machine->cpu); }
    if (machine->ram != NULL) { free(machine->ram); }
    if (machine->vga != NULL) { free(machine->vga); }
    if (machine->keyboard != NULL) { free(machine->keyboard); }
    if (machine->clock != NULL) { free(machine->clock); }
    free(machine);
}