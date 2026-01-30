#ifndef MY_INTERRUPT_HANDLERS_H
#define MY_INTERRUPT_HANDLERS_H

#include "ids/IRQ_list.h"


typedef struct KeyboardDev KeyboardDev;
typedef struct VGADev VGADev;
typedef struct BUS BUS;

void init_interrupt_handler(BUS *bus, VGADev *vga_dev, KeyboardDev *keyboard);

#endif
