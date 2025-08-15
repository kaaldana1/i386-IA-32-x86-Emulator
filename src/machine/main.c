#include "machine/boot.h"

int main(void)
{
    Program *p = create_program();
    start(p);
    return 1;
}
