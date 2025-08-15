#ifndef BUS_H
#define BUS_H
#include "devices.h"

typedef struct BUS BUS;

typedef int (*write_func) (void *device,  uint32_t data, uint32_t addr);
typedef int (*read_func) (void *device,  uint32_t *data, uint32_t addr);

BUS *create_bus(size_t max_devices);
void bus_destroy(BUS *bus);

int bus_register(BUS *bus, uint32_t base, uint32_t size, 
                read_func reader, write_func writer, void *device);

int bus_read(BUS *bus, uint32_t *data, uint32_t addr);
int bus_write(BUS *bus, uint32_t data, uint32_t addr);

#endif
