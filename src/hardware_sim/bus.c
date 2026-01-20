#include "hardware_sim/bus.h"


#define READ_SUCCESS 1
#define READ_FAIL 0
#define WRITE_SUCCESS 1
#define WRITE_FAIL 0


typedef struct 
{
    uint32_t base;
    uint32_t size;
    read_func read;
    write_func write;
    void *device;
} addr_table_entry;


struct BUS 
{
    addr_table_entry *map; 
    size_t map_size;
    size_t count;
};

BUS *create_bus(size_t max_devices) 
{
    BUS *bus = calloc(1, sizeof(BUS));
    bus->map_size = max_devices; // register ram, console port etc...
    bus->map = calloc(max_devices, sizeof(addr_table_entry));
    return bus;
}

void bus_destroy(BUS *bus) 
{
    free(bus->map);
    free(bus);
}

int bus_register(BUS *bus, uint32_t base, uint32_t size, read_func reader, write_func writer, void *device) 
{
    if (bus->count >= bus->map_size) { return 0; }
    bus->map[bus->count++] = (addr_table_entry){.base = base, .size = size, .read = reader, .write = writer, .device = device};
    return 1;
}


int bus_read(BUS *bus, uint32_t *data, uint32_t addr, size_t width) 
{
    size_t i = 0;
    while (i < bus->count) 
    {
        if (bus->map[i].base <= addr && bus->map[i].size + bus->map[i].base > addr) 
        {
            if (bus->map[i].read(bus->map[i].device, data, addr, width)) { return READ_SUCCESS; }
        }
        i++;
    }
    return READ_FAIL;
}


int bus_write(BUS *bus, uint32_t data, uint32_t addr, size_t width) 
{
    size_t i = 0;
    while (i < bus->count) 
    {
        if (bus->map[i].base <= addr && bus->map[i].size + bus->map[i].base > addr) 
        {
            if (bus->map[i].write(bus->map[i].device, data, addr, width)) { return WRITE_SUCCESS; }
        }
        i++;
    }
    return WRITE_FAIL;
}

