#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/syslimits.h>
#include <limits.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_ARGS 64

int parse_input(char *input, char *args[], int max_args) {
    int i = 0;              // Counter for how many arguments have been parsed
    char *p = input;        // Pointer to walk through the input string character by character

    // Continue parsing until the end of the string or we reach the max number of arguments
    while (*p && i < max_args - 1) {
        // Skip any leading spaces or tabs before the next argument
        while (*p == ' ' || *p == '\t')
            p++;
        // If we reach the end of the string after skipping spaces, stop parsing
        if (*p == '\0')
            break;
        // Check if the argument starts with a quote (single or double)
        if (*p == '"' || *p == '\'') {
            char quote = *p++;   // Remember which quote type and move past it
            args[i++] = p;       // The argument starts here
            // Move pointer until we find the matching closing quote
            while (*p && *p != quote)
                p++;
            // If closing quote found, replace it with null terminator to end the argument
            if (*p)
                *p++ = '\0';
            while(*p == ' ' || *p == 't') p++;
        } else {
            // Argument is not quoted
            args[i++] = p;       // The argument starts here
            // Move pointer until we hit a space, tab, or a quote
            while (*p && *p != ' ' && *p != '\t' && *p != '"' && *p != '\'')
                p++;
            // If we hit a delimiter, replace it with null terminator to end the argument
            if (*p)
                *p++ = '\0';
        }
    }

    args[i] = NULL;    // Null-terminate the array of arguments for execvp or similar
    return i;          // Return the number of arguments parsed
}


int main(int argc, char* argv[]) {
    char *input;
    char *args[MAX_ARGS];
    char cwd[PATH_MAX];

    while(1) {
        char prompt[PATH_MAX + 20];

        // prompt with curr dir
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            snprintf(prompt, sizeof(prompt), "\x1b[36mrysh:%s>\x1b[0m ", cwd);
        } else {
            snprintf(prompt, sizeof(prompt), "\x1b[36mrysh>\x1b[0m ");
        }

        input = readline(prompt);
        if(!input) break;  // Ctrl+D exits

        if(*input) add_history(input);
        // remove newline from input
        input[strcspn(input, "\n")] = '\0';

        // parse input into args
        int arg_count = parse_input(input, args, 64);

        // Skip empty input or no first argument
        if(strlen(input) == 0 || args[0] == NULL) {
            free(input);
            continue;
        }

        // built-in: exit
        if(strcmp(args[0], "exit") == 0) {
            free(input);
            break;
        }
        // built-in: cd
        else if(strcmp(args[0], "cd") == 0) {
            if(args[1] == NULL) {
                fprintf(stderr, "cd: missing argument\n");
            } else if (chdir(args[1]) != 0) {
                perror("cd");
            }
            free(input);
            continue;
        }
        // built-in: pwd
        else if(strcmp(args[0], "pwd") == 0) {
            if(getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("cwd");
            }
            free(input);
            continue;
        }
        // built-in: clear
        else if(strcmp(args[0], "clear") == 0) {
            printf("\033[H\033[J");
            free(input);
            continue;
        }

        // check background process
        int background = 0;
        if(arg_count > 0 && strcmp(args[arg_count - 1], "&") == 0) {
            background = 1;
            args[arg_count - 1] = NULL;
        }

        // handle redirection
        char *cmd_args[MAX_ARGS];
        char *outfile = NULL;
        char *infile = NULL;
        int append = 0;
        int cmd_argc = 0;
        int syntax_error = 0;

        for(int i = 0; args[i] != NULL; i++) {
            if(strcmp(args[i], ">") == 0 && args[i+1]) {
                if(args[i+1] == NULL) {
                    fprintf(stderr, "No file for ouput redirect\n");
                    syntax_error = 1;
                    break;
                }
                outfile = args[i+1];
                append = 0;
                i++;
            }
            else if(strcmp(args[i], ">>") == 0 && args[i+1]) {
                if(args[i+1] == NULL) {
                    fprintf(stderr, "No file for output redirect\n");
                    syntax_error = 1;
                    break;
                }
                outfile = args[i+1];
                append = 1;
                i++;
            }
            else if(strcmp(args[i], "<") == 0 && args[i+1]) {
                if(args[i+1] == NULL) {
                    fprintf(stderr, "No file for input redirect\n");
                    syntax_error = 1;
                    break;
                }
                infile = args[i+1];
                i++;
            }
            else {
                cmd_args[cmd_argc++] = args[i];
            }
        }
        cmd_args[cmd_argc] = NULL;

        // fork and execute
        pid_t pid = fork();
        if(pid == 0) {
            if(infile) {
                int fd_in = open(infile, O_RDONLY);
                if(fd_in < 0) {
                    perror("open input");
                    exit(1);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
            if(outfile) {
                int fd = open(outfile, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);
                if(fd < 0) {
                    perror("open");
                    exit(1);
                }
                dup2(fd , STDOUT_FILENO);
                close(fd);
            }
            execvp(cmd_args[0], cmd_args);
            perror("execvp");
            exit(1);
        } else if(pid > 0) {
            if (!background) {
                int status;
                waitpid(pid, &status, 0);
            } else {
                printf("[background pid: %d]\n", pid);
            }
        } else {
            perror("fork");
        }

        if(syntax_error) {
            free(input);
            continue;
        }
    }

    printf("Cya!\n");
    return 0;
}
