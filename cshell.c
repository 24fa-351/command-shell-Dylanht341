#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include "cshell.h"

typedef struct {
    char *name;
    char *value;
} EnvVar;

EnvVar my_env[100];
int env_count = 0;

void print_prompt() {
    printf("xsh# ");
    fflush(stdout);
}

void parse_input(char *input, char **args, int *arg_count) {
    *arg_count = 0;
    char *token = strtok(input, " ");

    while (token != NULL && *arg_count < MAX_ARGS - 1) {
        args[(*arg_count)++] = token;
        token = strtok(NULL, " ");
    }
    args[*arg_count] = NULL;
}

void execute_command(char **args, int arg_count) {
    if (strcmp(args[0], "cd") == 0) {
        change_directory(args);
    } else if (strcmp(args[0], "pwd") == 0) {
        print_working_directory();
    } else if (strcmp(args[0], "set") == 0) {
        handle_set(args);
    } else if (strcmp(args[0], "unset") == 0) {
        handle_unset(args);
    } else {
        int background = 0;

        if (arg_count > 1 && strcmp(args[arg_count - 1], "&") == 0) {
            background = 1;
            args[--arg_count] = NULL;
        }

        for (int i = 0; i < arg_count; i++) {
            if (strcmp(args[i], "|") == 0) {
                handle_piping(args, arg_count);
                return;
            } else if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0) {
                handle_redirection(args, arg_count);
                return;
            }
        }
        execute_external(args, background);
    }
}

void change_directory(char **args) {
    if (args[1] == NULL || chdir(args[1]) != 0) {
        perror("cd");
    }
}

void print_working_directory() {
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

void handle_set(char **args) {
    for (int ix = 0; ix < env_count; ix++) {
        if (strcmp(my_env[ix].name, args[1]) == 0) {
            free(my_env[ix].value);
            my_env[ix].value = strdup(args[2]);
            return;
        }
    }

    if (env_count < 100) {
        my_env[env_count].name = strdup(args[1]);
        my_env[env_count].value = strdup(args[2]);
        env_count++;
    }
}

void handle_unset(char **args) {
    for (int ix = 0; ix < env_count; ix++) {
        if (strcmp(my_env[ix].name, args[1]) == 0) {
            free(my_env[ix].name);
            free(my_env[ix].value);
            my_env[ix] = my_env[--env_count];
            return;
        }
    }
}

char *substitute_variables(char *input) {
    char *result = malloc(MAX_INPUT);
    result[0] = '\0';
    char *start = input;
    char *dollar = strchr(start, '$');
    int found = 0;

    while (dollar != NULL) {
        strncat(result, start, dollar - start);
        char *var_start = dollar + 1;
        char var_name[128] = {0};

        sscanf(var_start, "%127[^ \t\n$]", var_name);
        for (int ix = 0; ix < env_count; ix++) {
            if (strcmp(my_env[ix].name, var_name) == 0) {
                strcat(result, my_env[ix].value);
                found = 1;
                break;
            }
        }
        if (!found) {
            strcat(result, dollar);
        }

        start = var_start + strlen(var_name);
        dollar = strchr(start, '$');
    }
    strcat(result, start);
    return result;
}

void handle_piping(char **args, int arg_count) {
    int pipefd[2];
    pid_t pid;

    for (int ix = 0; ix < arg_count; ix++) {
        if (strcmp(args[ix], "|") == 0) {
            args[ix] == NULL;
            if (pipe(pipefd) == -1) {
                perror("Pipe failed");
                return;
            }

            if ((pid = fork()) == -1) {
                perror("Fork failed");
                return;
            }

            if (pid == 0) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
                execute_command(args, ix);
                exit(0);
            } else {
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
                execute_command(&args[ix + 1], arg_count - ix - 1);
                wait(NULL);
                return;
            }
        }
    }
}

void handle_redirection(char **args, int arg_count) {
    for (int ix = 0; ix < arg_count; ix++) {
        if (strcmp(args[ix], ">") == 0 || strcmp(args[ix], "<") == 0) {
            int fd;

            if (strcmp(args[ix], ">") == 0) {
                fd = open(args[ix + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

                if (fd == -1) {
                    perror("Open failed");
                    return;
                }
                dup2(fd, STDOUT_FILENO);
            } else if (strcmp(args[ix], "<") == 0) {
                fd = open(args[ix + 1], O_RDONLY);

                if (fd == -1) {
                    perror("Open failed");
                    return;
                }
                dup2(fd, STDIN_FILENO);
            }

            close(fd);
            args[ix] = NULL;
            execute_command(args, ix);
            return;
        }
    }
}

void execute_external(char **args, int background) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror("Execvp failed");
        exit(1);
    } else if (!background) {
        waitpid(pid, NULL, 0);
    }
}

char *find_command_in_path(const char *command) {
    char *path = getenv("PATH");
    char *token = strtok(path, ":");
    static char full_path[1024];

    while (token != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", token, command);
        if (access(full_path, X_OK) == 0) {
            return full_path;
        }
        token = strtok(NULL, ":");
    }
    return NULL;
}
