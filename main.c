#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cshell.h"

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    int arg_count;

    while(1) {
        print_prompt();
        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            break;
        }

        char *sub_input = substitute_variables(input);
        parse_input(sub_input, args, &arg_count);

        if (arg_count > 0) {
            execute_command(args, arg_count);
        }

        free(sub_input);
    }
    return 0;
}