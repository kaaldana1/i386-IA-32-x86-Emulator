#include <stdlib.h>
#include "core/clock.h"

Clock *init_clock()
{
    Clock *clock = (Clock*)calloc(1, sizeof(Clock));
    return clock;
}


Clock read_clock(const Clock *clock)
{
    return *clock;
}

void destroy_clock(Clock *clock)
{
    free(clock);
}


