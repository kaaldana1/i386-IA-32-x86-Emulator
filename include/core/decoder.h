#ifndef DECODER_H
#define DECODER_H

#include "cpu_internal.h"
#include "tables/decode_tables.h"

Instruction *decoder(uint8_t *encoded_instr);

#endif
