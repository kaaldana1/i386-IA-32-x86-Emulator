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
            printf("Usage: \n \t emulator --program <path> [options] \n\n");
            printf("Required: \n \t --program <path> \t\t Path to the program file to load \n\n");
            printf("Options: \n");
            printf("\t --ui \t\t\t\t Enable the ncurses-based UI (default) \n");
            printf("\t --headless \t\t\t Run the emulator without UI (in progress) \n\n");
            printf("\t --delay <milliseconds> \t Set the delay between instruction executions in UI mode (the default is no delay) \n\n");
            printf("Runtime controls (for UI mode): \n");
            printf("\t Press 'q' or ctrl-c to quit the emulator\n");
            exit(1);
        }

        if (strcmp(argv[i], "--ui") == 0)
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
            
            if (i + 1 < argc && strcmp(argv[++i], "--ui") == 0)
            {
                ui_on = true;
            }

            if (ui_on)
            {
                i++;
                long delay = 0;
                if (i + 1 < argc && strcmp(argv[i], "--delay") == 0)
                {
                    i++;
                    delay = strtol(argv[i], NULL, 10);
                }

                if (init_ui(delay) == 0) 
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
