#include "machine/gdt.h"

#define ACCESSED            0x01
#define RW                  0x02
#define DC                  0x04
#define EXECUTABLE          0x08
#define SEG_DESCTYPE(x)     ((x) << 0x04) // Descriptor type, 0 for system, 1 for code/data
#define DESC_DPL0           0x00
#define DESC_DPL1           0x20
#define DESC_DPL2           0x40
#define DESC_DPL3           0x60
#define PRESENT             0x80

#define AVL                 0x01
#define DB_32_BIT           0x04
#define GRAIN_BYTE          0x00
#define GRAIN_4KiB          0x08

#define USER_DATA_RW     (PRESENT | DESC_DPL3 | SEG_DESCTYPE(1) |  RW) // 0xF2
#define USER_CODE_RX     (PRESENT | DESC_DPL3 | SEG_DESCTYPE(1) | EXECUTABLE | RW) //0xFA
#define KERNEL_CODE_RX   (PRESENT | DESC_DPL0 | SEG_DESCTYPE(1) | EXECUTABLE | RW) //0x9A
#define KERNEL_DATA_RW   (PRESENT | DESC_DPL0 | SEG_DESCTYPE(1) | RW)   //0x92

inline static uint64_t create_descriptor(uint32_t base, uint32_t limit, 
                                         uint8_t access, uint8_t flags) 
{
    uint64_t base_low = (uint64_t)base & 0xFFFF;
    uint64_t base_mid = ((uint64_t) base >> 16) & 0xFF;
    uint64_t base_high = ((uint64_t)base >> 24) & 0xFF;

    uint64_t limit_low = (uint64_t)limit & 0xFFFF;
    uint64_t flag_and_limit = (((uint64_t)flags & 0x0F) << 4) | (((uint64_t)limit >> 16) & 0x0F);

    return   ((base_high << 56) | (flag_and_limit << 48) | ((uint64_t)access << 40) | (base_mid << 32) | (base_low << 16) | limit_low);
}

inline static int create_gdt_entry(BUS *bus,  uint32_t addr, uint64_t descriptor)
{
    uint32_t low = (uint32_t)(descriptor & 0xFFFFFFFF); // limit low, base low
    uint32_t high = (uint32_t)((descriptor >> 32) & 0xFFFFFFFF); // base mid, access byte, flag (4 bits), limit high (4 bits), base high
    bus_write(bus, low, addr);
    bus_write(bus, high, addr + 4);
    return 1;
}

int create_gdt(BUS *bus, uint32_t table_addr) 
{
    create_gdt_entry(bus, table_addr, create_descriptor(0, 0, 0x00, 0x00));
    create_gdt_entry(bus, (table_addr + 8), create_descriptor(0x0000, 0x07FF, KERNEL_CODE_RX, 0x04));
    create_gdt_entry(bus, (table_addr + 16), create_descriptor(0x0800, 0x07FF, KERNEL_DATA_RW, 0x04));
    create_gdt_entry(bus, (table_addr + 24), create_descriptor(0x1000, 0x27FF, USER_CODE_RX, 0x04));
    create_gdt_entry(bus, (table_addr + 32), create_descriptor(0x1800, 0x27FF, USER_DATA_RW, 0x04));
    return 1;
}

