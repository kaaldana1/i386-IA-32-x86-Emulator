#ifndef MY_INTERRUPT
#define MY_INTERRUPT


#include "ids/IRQ_list.h"

typedef struct KeyboardDev KeyboardDev;
typedef struct VGADev VGADev;


void init_interrupt_controller(KeyboardDev *keyboard_dev, VGADev *vga_dev);


#endif

