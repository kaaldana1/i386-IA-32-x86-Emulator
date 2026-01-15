#include <locale.h>
#include <ncurses.h>
#include "ui/display.h"
#include "machine/boot.h"


int main(void)
{

    setlocale(LC_ALL, "");
    init_ui();

    Program *p = create_program();
    start(p);
    destroy_window();             


    return 1;
}
