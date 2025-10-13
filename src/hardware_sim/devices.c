#include "hardware_sim/devices.h"
#include "machine/display_api.h"

#define DWORD_SIZE 4 

struct RAMDev 
{
    uint8_t ram_16kb[RAM_SIZE];
};

struct CONSOLEDev
{
    uint8_t data[256];
};

RAMDev *init_ram(void)
{
    RAMDev *ram_dev = (RAMDev*)calloc(1, sizeof(RAMDev));
    memset(ram_dev->ram_16kb, 0x00, RAM_SIZE);
    return ram_dev;
}

CONSOLEDev *init_console(void)
{
    CONSOLEDev *c = (CONSOLEDev*)calloc(1, sizeof(CONSOLEDev));
    return c;
}

int ram_write_byte(RAMDev *r, uint8_t value, uint32_t address) 
{
    r->ram_16kb[address] = value;
    return 1;
}

int ram_write_word(RAMDev *r, uint16_t value, uint32_t address) 
{
    ram_write_byte(r, (uint8_t)value, address);
    ram_write_byte(r, (uint8_t)(value >> 8), address + 1);
    return 1;
}

int ram_write_dword(void *ram_dev, uint32_t value, uint32_t address) 
{
    RAMDev *r = ram_dev;
    if (address > RAM_SIZE - 4) { return 0; }
    ram_write_word(r, (uint16_t)(value & 0xFFFF), address);
    ram_write_word(r, (uint16_t)(value >> 16), address + 2);
    return 1;
} 

int ram_read_byte (RAMDev *r, uint8_t *value, uint32_t address)
{
    *value = r->ram_16kb[address];
    return 1;
}

int ram_read_word(RAMDev *r, uint16_t *value, uint32_t address) 
{
    uint8_t low, high;
    ram_read_byte(r, &low, address);
    ram_read_byte(r, &high, address + 1);
    *value = (uint16_t)low | ((uint16_t)high << 8);
    return 1;
}


int ram_read_dword(void *ram_dev, uint32_t *value, uint32_t address) 
{
    RAMDev *r = ram_dev;
    if (address > RAM_SIZE - 4) { return 0; }
    uint16_t low, high;
    ram_read_word(r, &low, address);
    ram_read_word(r, &high, address + 2);
    *value = (uint32_t)low | ((uint32_t)high << 16);
    return 1;
}

int ram_read(RAMDev *r, uint32_t *value, uint32_t address, size_t width)
{
    RAMDev *ram = r;
    if (address > RAM_SIZE - 4) { return 0; }
    if (width == 8)
        return ram_read_byte(ram, (uint8_t *)value, address);
    else if (width == 16)
        return ram_read_word(ram, (uint16_t *)value, address);
    else if (width == 32)
        return ram_read_dword(ram, value, address);
    return 1;
}

int ram_write(RAMDev *r, uint32_t value, uint32_t address, size_t width)
{
    RAMDev *ram = r;
    if (address > RAM_SIZE - 4) { return 0; }
    if (width == 8) 
        return ram_write_byte(r, (uint8_t)value, address);
    else if (width == 16)
        return ram_write_word(r, (uint16_t)value, address);
    else if (width == 32)
        return ram_write_dword(r, value, address);
    return 1;
}

int console_read_stub(void *device, uint32_t *value, uint32_t address, size_t width)
{
    (void)device; (void)value; address = 0; width = 0;
    return 1;
}

int console_write(void *device, uint32_t data, uint32_t addr, size_t width) 
{
    (void)device; addr = 0; width = 0;
    //printwtw("\t+----------------+\n");
    //printwtw("\t| CONSOLE PRINT: | %d\n", data);
    //printwtw("\t+----------------+\n");
    return 1;
}

