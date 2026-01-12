#include "core/interrupt/interrupt_controller.h"
#include "hardware_sim/devices_internal.h"


InterruptController irc = {{0}, {0}};

void interrupt_controller(IRQNumber irq_num)
{
    irc.pending[irq_num] = 1;
}

void init_interrupt_controller(KeyboardDev *keyboard_dev, VGADev *vga_dev)
{
    Interrupter *ki = &keyboard_dev->interrupter;
    Interrupter *vi = &vga_dev->interrupter;
    ki->interrupt_line = interrupt_controller;
    vi->interrupt_line = interrupt_controller;
    ki->irq_num = KB_IRQ;
    vi->irq_num = VGA_IRQ;

    irc.enabled[ki->irq_num] = 1;
    irc.enabled[vi->irq_num] = 1;
}

