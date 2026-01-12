#ifndef MY_INTERRUPT
#define MY_INTERRUPT


#include <stdbool.h>
#include "ids/IRQ_list.h"

typedef struct KeyboardDev KeyboardDev;
typedef struct VGADev VGADev;

typedef struct 
{
    bool enabled[IRQ_COUNT];
    bool pending[IRQ_COUNT];
} InterruptController;

void init_interrupt_controller(KeyboardDev *keyboard_dev, VGADev *vga_dev);

extern InterruptController irc;

#endif

