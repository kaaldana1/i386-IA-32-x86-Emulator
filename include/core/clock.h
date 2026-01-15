#ifndef MY_TIME
#define MY_TIME

typedef long Clock;

Clock *init_clock();
Clock read_clock(const Clock *clock);
void destroy_clock(Clock *clock);


#endif
