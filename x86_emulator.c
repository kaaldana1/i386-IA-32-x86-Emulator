#include "x86_emulator.h"

int main() {
    Program test;
    if (parse_file(&test)) {
        print_contents(&test);
    }

    if(interpreter(&test)) {
        printf("yay");
    }
    return 0;
}