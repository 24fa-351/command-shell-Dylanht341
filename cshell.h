#ifndef CSHELL_H
#define CSHELL_H

#define MAX_INPUT 1024
#define MAX_ARGS 128

void print_prompt();
void parse_input(char *input, char **args, int *arg_count);
void execute_command(char **args, int arg_count);
void change_directory(char **args);
void print_working_directory();
void handle_set(char **args);
void handle_unset(char **args);
char *substitute_variables(char *input);
void handle_piping(char **args, int arg_count);
void handle_redirection(char **args, int arg_count);
void execute_external(char **args, int background);
char *find_command_in_path(const char *command);

#endif