#include <ncurses.h>
#include "machine/display.h"
#include "machine/boot.h"


int main(void)
{

    int chr;

    while ((chr = getch()) != 'q') 
    {
        init_ui();
        Program *p = create_program();
        start(p);
    }

    destroy_window();             // End ncurses mode
    return 1;
}
