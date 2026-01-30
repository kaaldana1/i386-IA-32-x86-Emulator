#ifndef INT_CPU_INTERRUPTS_H
#define INT_CPU_INTERRUPTS_H


#include <stdbool.h>
#include "ids/IRQ_list.h"

typedef int IRQNumber;

typedef struct 
{
    bool enabled[IRQ_COUNT];
    bool pending[IRQ_COUNT];
} InterruptController;
extern InterruptController irc;

typedef void (*IRQ_handler)();
extern IRQ_handler irq_handler_table[IRQ_COUNT];

#endif
