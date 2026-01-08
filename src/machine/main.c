#include <locale.h>
#include <ncurses.h>
#include "ui/display.h"
#include "machine/boot.h"


int main(void)
{

    int chr;

    #ifdef NCURSES_ON
    setlocale(LC_ALL, "");
    while ((chr = getch()) != 'q') 
    {
        init_ui();

        Program *p = create_program();
        start(p);
    }
    destroy_window();             // End ncurses mode
    #endif

    #ifdef DEBUG
    Program *p = create_program();
    start(p);
    #endif

    return 1;
}
