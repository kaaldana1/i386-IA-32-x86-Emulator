#include "machine/boot.h"
#include "hardware_sim/memmap.h"
#include "ui/display.h"
#include "core/interrupt/interrupt_controller.h"
#include "core/interrupt/interrupt_handlers.h"


typedef struct 
{
    CPU *cpu;
    BUS *bus;
} Machine;

static Machine *boot_sequence(Program *p);
static int create_addr_space(RAMDev *ram, BUS *bus, CONSOLEDev *c, VGADev *v, KeyboardDev *kb);

int start(Program *p) 
{
    Machine *m = boot_sequence(p);
    interpreter(m->cpu, m->bus);
    return 1;
}

static Machine *boot_sequence(Program *p) 
{
    //TODO: Define addresses here, parameterize the addresses. Dont let functions hide them
    RAMDev *ram = init_ram(); // 16 kib array
    CONSOLEDev *c = init_console();
    VGADev *v = init_vga();
    KeyboardDev *kb = init_keyboard();

    // BUS has an address table, an entry in the table expects:
        /*
            uint32_t base;
            uint32_t size;
            read_func read;
            write_func write;
            void *device;
        */
    BUS *bus = create_bus(MAX_DEVICES);
    
    init_interrupt_handler(bus, v, kb);
    init_interrupt_controller(kb, v);

    // Register devices into the bus:
        // Ram starts at 0x0, size is 16kb
        // Console port is at address 0x4000
    create_addr_space(ram, bus, c, v, kb); 
    
    // Creates GDT at GDT base
        /* Descriptors:
            (0x0000, 0x07FF, KERNEL_CODE_RX
            (0x0800, 0x07FF, USER_CODE_RX 
            (0x1000, 0x27FF, USER_DATA_RW  
            (0x1800, 0x27FF, STACK
        */
    create_gdt(bus, GDT_BASE_ADDR);

    CPU *cpu = create_cpu();

    // Sets GDTR base pointer in CPU
    // Sets selector bits in Segment Register:
        /*
    cpu->segment_registers[CS].selector = 0x1 -> Indexes into User Code descriptor
    cpu->segment_registers[DS].selector = 0x1 -> Indexes into User Data desciptor
    cpu->segment_registers[SS].selector = 0x2 -> Indexes into User Stack desciptor
        */
    cpu_protected_mode_reset(bus, cpu, GDT_BASE_ADDR, GDT_SIZE);
    init_execution_table();
    load_program(bus, p, USER_CODE_BASE_ADDR);

    Machine *m = (Machine *)calloc(1, sizeof(Machine));
    *m = (Machine){.bus = bus, .cpu = cpu };


    return m;
}

static int create_addr_space(RAMDev *ram,  BUS *bus, CONSOLEDev *c, VGADev *v, KeyboardDev *kb) 
{
    bus_register(bus, RAM_BASE_ADDR, RAM_SIZE, ram_read, ram_write, ram);
    bus_register(bus, CONSOLE_BASE_ADDR, CONSOLE_SIZE,  console_read_stub, console_write, c); 
    bus_register(bus, VGA_BASE_ADDR, VGA_SIZE, vram_read_stub, vram_write, v);
    bus_register(bus, KEYBOARD_BASE_ADDR, KEYBOARD_SIZE, keyboard_read, keyboard_write, kb);
    return 1;
}


