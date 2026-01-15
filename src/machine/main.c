#include <locale.h>
#include <ncurses.h>
#include "ui/display.h"
#include "machine/boot.h"


int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    int i = 0;
    while (i < argc)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            printf("--program <path_to_your_program> \n");
            exit(1);
        }

        if (strcmp(argv[i], "--program") == 0 && i + 1 < argc)
        {
            i++;
            Program *p = create_program(argv[i]);
            if (p == NULL || p->filename == NULL) {
                printf("Error: Could not load program %s\n", argv[i]);
                exit(1);
            }

            init_ui();

            if (start(p) == 0) {
                destroy_window();             
                exit(1);
            }
            destroy_window();             
        }
        i++;
    }



    return 1;
}
