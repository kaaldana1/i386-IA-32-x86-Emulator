#ifndef GDT_H
#define GDT_H

#include "hardware_sim/bus.h"

#define GDT_BASE_ADDR       0xD6
#define GDT_SIZE            40 // bytes
#define USER_CODE_BASE_ADDR 0x1000
#define USER_DATA_BASE_ADDR 0x1800

// https://wiki.osdev.org/GDT_Tutorial

int create_gdt(BUS *bus, uint32_t table_addr);

#endif
