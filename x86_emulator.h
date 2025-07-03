#ifndef X86_EMULATOR_H
#define X86_EMULATOR_H

#include "execute.h"
// EIP is the instruction pointer
int initialize(Program *p);
int interpreter();

//=========
#endif