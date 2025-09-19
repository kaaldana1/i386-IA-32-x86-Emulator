#include "machine/boot.h"

#define MAX_DEVICES 2 // for just console and ram
#define START_EIP 0x1000 // same as the user code base in gdt
#define START_ESP (GDT_BASE_ADDR - 1)


typedef struct 
{
    CPU *cpu;
    BUS *bus;
} Machine;

static Machine *boot_sequence(Program *p);
static int create_addr_space(RAMDev *ram, BUS *bus, CONSOLEDev *c);

int start(Program *p) 
{
    Machine *m = boot_sequence(p);
    interpreter(m->cpu, m->bus);
    return 1;
}

static Machine *boot_sequence(Program *p) 
{
    RAMDev *ram = init_ram(); // 16 kib array
    CONSOLEDev *c = init_console();
    BUS *bus = create_bus(MAX_DEVICES);
    create_addr_space(ram, bus, c);
    create_gdt(bus, GDT_BASE_ADDR);
    CPU *cpu = create_cpu();
    cpu_protected_mode_reset(bus, cpu, START_EIP, START_ESP, GDT_BASE_ADDR, GDT_SIZE);
    init_execution_table();
    load_program(bus, p, USER_CODE_BASE_ADDR);

    Machine *m = (Machine *)calloc(1, sizeof(Machine));
    *m = (Machine){.bus = bus, .cpu = cpu };
    return m;
}

static int create_addr_space(RAMDev *ram,  BUS *bus, CONSOLEDev *c) 
{
    bus_register(bus, 0x0000, 0x3FFF, ram_read, ram_write, ram);
    bus_register(bus, 0x4000, 0x1,  console_read_stub, console_write, c); 
    return 1;
}


