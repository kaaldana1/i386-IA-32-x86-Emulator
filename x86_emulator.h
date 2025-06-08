#ifndef X86_EMULATOR_H
#define X86_EMULATOR_H

#include "program_loader.h"
#include "execute.h"
// EIP is the instruction pointer
int interpreter(Program *p);

//=========
#endif