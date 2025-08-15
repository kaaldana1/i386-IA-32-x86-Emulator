#ifndef BOOT_H
#define BOOT_H

#include "hardware_sim/bus.h"
#include "hardware_sim/devices.h"
#include "core/cpu.h"
#include "machine/gdt.h"
#include "machine/program_loader.h"


int start(Program *p);

#endif
