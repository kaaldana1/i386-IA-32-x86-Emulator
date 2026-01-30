
#include "ui/display_api.h"
#include "core/int_cpu_interrupts.h"
#include "hardware_sim/devices_internal.h"
#include "hardware_sim/memmap.h"
#include "hardware_sim/bus.h"
#include "hardware_sim/interrupt_handlers.h"


typedef struct 
{
    VGADev *vga_dev;
    KeyboardDev *keyboard;
    BUS *bus;
} Devices; 

Devices devices = {NULL, NULL, NULL};

void keyboard_handler();
void vga_handler();

void init_interrupt_handler(BUS *bus, VGADev *vga_dev, KeyboardDev *keyboard)
{
    devices.keyboard = keyboard;
    devices.vga_dev = vga_dev;
    devices.bus = bus;
}

// this should point to the actual handlers from guest code

IRQ_handler irq_handler_table[IRQ_COUNT] = {
    [KB_IRQ] = keyboard_handler,
    [VGA_IRQ] = vga_handler
};

// These should be guess code but for now its part of the emulator
void keyboard_handler()
{
    KeyboardDev *kb =  devices.keyboard;
    uint8_t data = 0;
    kb->dequeue(kb->keyboard_buffer, &data, &kb->keystrokes_in_queue, sizeof(kb->keyboard_buffer));

    bus_write(devices.bus, data, KEYBOARD_BASE_ADDR, 8);
}

void vga_handler()
{
    if (ui_on)
        machine_state.ui_callbacks.ui_update_screen();
}


