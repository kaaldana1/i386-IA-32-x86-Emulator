#include <locale.h>
#include <ncurses.h>
#include "ui/display.h"
#include "ui/display_api.h"
#include "machine/boot.h"
#include "ids/return_code_list.h"


int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    int i = 0;
    while (i < argc)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            printf("Start the emulator:                         --program <path_to_your_program> \n\n");
            printf("IMPORTANT: PLEASE TURN ON UI MODE FOR NOW--headless mode currently in the works\n");
            printf("UI mode (headless mode default):            --UI_ON \n\n");

            printf("Press 'q' or ctrl-c to quit the emulator\n");
            exit(1);
        }

        if (strcmp(argv[i], "--UI_ON") == 0)
        {
            ui_on = true;
        }

        if (strcmp(argv[i], "--program") == 0 && i + 1 < argc)
        {
            i++;
            Program *p = create_program(argv[i]);
            if (p == NULL || p->filename == NULL) 
            {
                printf("Error: Could not load program %s\n", argv[i]);
                exit(1);
            }
            
            if (i + 1 < argc && strcmp(argv[++i], "--UI_ON") == 0)
            {
                ui_on = true;
            }

            if (ui_on)
            {
                if (init_ui() == 0) 
                {
                    printf("Error: UI failed to initialize\n");
                    exit(1);
                }
            }

            int status = start(p);
            destroy_window();             
            fflush(stdout);

            switch (status)
            {
                case MACHINE_TURN_OFF:
                    printf("Machine turned off successfully\n");
                    break;
                case BUS_READ_FAILURE:
                    printf("Program terminated because of bus read failure\n");
                    break;
                case BUS_WRITE_FAILURE:
                    printf("Program terminated because of bus write failure.\n");
                    break;
                case BAD_DECODE:
                    printf("Program terminated because of instruction decode.\n");
                    break;
                case BAD_EXECUTION:
                    printf("Program terminated because of bad instruction execution\n");
                    break;
                case UNIMPLEMENTED_INSTRUCTION:
                    printf("Program terminated because instruction has not yet been implemented.\n");
                    break;
                case FAILED_SEGBOUND_CHECK:
                    printf("Program terminated because of segment bounds check failure\n");
                case INVALID_EFFECTIVE_ADDRESS:
                    printf("Program terminated because of an invalid effective address calculation\n");
                    break;

                default:
                    break;
            }
            
            return 1;

        }
        i++;
    }

    return 1;
}
