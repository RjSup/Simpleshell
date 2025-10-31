#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_pid_t.h>
#include <unistd.h>
#include <sys/wait.h>

void parse_input(char *input, char **args) {
    // split input into args
    int i = 0;
    // token is data between whitespaces
    char *token = strtok(input, " \t\r\n");
    // while tokens still exist
    while(token != NULL && i < 63) {
        // increment the arguments and store in token
        args[i++] = token;
        // keep reading tokens
        token = strtok(NULL, " \t\r\n");
    }
    // null terminated array of string for exe()
    args[i] = NULL;
}

int main(int argc, char* argv[]) {
    char input[1024];
    char *args[64];
    char cwd[1024];

    while(1) {
        printf("endvrsh> ");
        // flush out data in buffer
        fflush(stdout);

        // take input from stdin put it in input
        if(fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // remove newline from input data - find newline - chnage it for end of line
        input[strcspn(input, "\n")] = '\0';

        // skip cos line is empty
        if(strlen(input) == 0) {
            continue;
        }

        parse_input(input, args);

        // if the first argument doesnt exist
        if(args[0] == NULL) {
            continue;
        }

        // if first argument is exit
        if(strcmp(args[0], "exit") == 0) {
            break;
        // if first argument is cd
        } else if(strcmp(args[0], "cd") == 0) {
            // if second argument doesnt exist
            if(args[1] == NULL) {
                // error
                fprintf(stderr, "cd: missing argument\n");
            // if second argument exists change dir with first argument
            } else if (chdir(args[1]) != 0) {
                perror("cd");
            }
            continue;
        // if 1st argument == pwd
        } else if(strcmp(args[0], "pwd") == 0) {
            // get path sizeof path into cwd
            if(getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("cwd");
            }
            continue;
        // echo prints out the arguments put after "echo"
        } else if(strcmp(args[0], "echo") == 0) {
            // skip argument 0 cos its "echo"
            int start = 1;
            // whether to print a new line or nah. default yes
            int newLine = 1;

            // check for "-n" flag
            if(args[1] && strcmp(args[1], "-1") == 0) {
                // no newline needed
                newLine = 0;
                // use second args cos first is "echo"
                start = 2;
            }

            // print all arguments starting from second arg until last arg
            for (int j = start; args[j] != NULL; j++) {
                printf("%s", args[j]);
                // if another arg exists add space
                if (args[j + 1] != NULL) {
                    printf(" ");
                }
            }

            // if newline flag still 1 print it
            if(newLine) {
                printf("\n");
            }

            continue;
        }

        // create new process
        pid_t pid = fork();
        if(pid == 0) {
            // in the child this replaces the child program with the command in first argument
            if(execvp(args[0], args) == -1) {
                perror("execvp");
            }
            exit(EXIT_FAILURE);
        // wait for child process to finish before continue
        } else if(pid > 0) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            perror("fork");
        }
    }

    printf("Cya!\n");

    return 0;
}
