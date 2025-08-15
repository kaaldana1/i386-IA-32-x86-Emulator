#ifndef OPERAND_ADDR_FORM_H
#define OPERAND_ADDR_FORM_H

#include "ids/register_ids.h"

typedef struct
{
    GPR_type effective_addr_register;
    GPR_type src_register;
} Operand_addr_form;

#endif
