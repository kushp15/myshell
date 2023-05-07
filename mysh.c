#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <glob.h>
#include <errno.h>
#include <sys/stat.h>


// Define variables
#define MAX_ARGS 256  
#define MAX_PIPES 10 // Assuming maxium pipe commands witll 10
#define MAX_PATH 6


// Given File path to
const char* paths[MAX_PATH] = {
    "/usr/local/sbin",
    "/usr/local/bin",
    "/usr/sbin",
    "/usr/bin",
    "/sbin",
    "/bin"
};

// Handle PWD
void handle_pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        fprintf(stderr, "Error: %s\n", strerror(errno));
    }
}


// Interactive mode
int interactive_mode() {
    char input[256];
    char* args[MAX_ARGS];
    char* pipes[MAX_PIPES][MAX_ARGS];
    char* ptr;
    int p_count = 0;
    int arg_count = 0;
    int status;
    pid_t pid;
    int fd[2];


    // Welcome Prompt
    printf("*** Welcome to MyShell ****(\n");
    while (1) {
        // Print Promopt for the user
        printf("mysh > ");
        // Getting the args in the user input
        fgets(input, MAX_ARGS, stdin);
        input[strlen(input)-1] = '\0'; 
        // Split input into arguments
        ptr = strtok(input, " ");

        // Arg count, total no of arg passed by the user
        arg_count = 0;
            while (ptr != NULL && arg_count < MAX_ARGS-1) {
                args[arg_count++] = ptr;
                ptr = strtok(NULL, " ");
            }
            args[arg_count] = NULL;

            // Changing directory us cd
            // If user inputs only cd will be directed to home page
            if (strcmp(args[0], "cd") == 0) {
                if (chdir(args[1]) != 0) {
                    perror("chdir() error");
                }
                continue;
            }   

            // Check working directory using pwd
            if(strcmp(args[0], "pwd") == 0){
                handle_pwd();
                continue;
            }

            // Exit the shell command type 'Exit', exited with the message of GoodBye!!
            if (strcmp(args[0], "Exit") == 0) {
                printf("Goodbye \n");
                exit(EXIT_SUCCESS);
                waitpid(pid, &status, 0);
            }

            // Check the user input contains any pipe '|', if contains then handle pipe
            int has_pipe = 0;
            for (int i = 0; i < arg_count; i++) {
                if (strcmp(args[i], "|") == 0) {
                    has_pipe = 1;
                    break;
                }
            }

            // Check the user input contains any pipe '*', if contains then handle Wildcard
            int has_wildcard = 0;
            for (int i = 0; i < arg_count; i++) {
                if (strchr(args[i], '*')) {
                    has_wildcard = 1;
                    break;
                }
            }

            // If the input contains the "*", if statement will run.
            if (has_wildcard) {
                int p_ind = 0;
                while (p_ind < p_count) {
                    glob_t ans;
                    memset(&ans, 0, sizeof(ans));
                    int flags = 0;
                    int a_ind = 0;
                    while (pipes[p_ind][a_ind] != NULL) {
                        glob(pipes[p_ind][a_ind], flags, NULL, &ans);
                        a_ind++;
                    }
                    a_ind = 0;
                    for (int i = 0; i < ans.gl_pathc && a_ind < MAX_ARGS-1; i++) {
                        pipes[p_ind][a_ind++] = ans.gl_pathv[i];
                    }
                    pipes[p_ind][a_ind] = NULL;
                    globfree(&ans);
                    p_ind++;
                }
            }


            // Checking how many pipe command are there, if there are more than 1 than it will split into
            // different commands and executes acordningly
            if (has_pipe) {
                // making an counter of hwo many pipe presents p_count
                p_count = 0;
                // Checking at what location pipe is presents with i_ind
                int a_ind = 0; 
                // running the while loop
                while (args[a_ind] != NULL && p_count < MAX_PIPES) {
                    int pipe_arg_count = 0;
                    while (args[a_ind] != NULL && strcmp(args[a_ind], "|") != 0
                        && pipe_arg_count < MAX_ARGS-1) {
                        pipes[p_count][pipe_arg_count++] = args[a_ind++];
                    }
                    pipes[p_count][pipe_arg_count] = NULL;
                    if (args[a_ind] != NULL && strcmp(args[a_ind], "|") == 0) {
                        a_ind++;
                    }
                    p_count++;
                }

                // Executing pipes
                int fd[2];
                int in = STDIN_FILENO;
                for (int i = 0; i < p_count; i++) {
                    pipe(fd);
                    pid_t pid = fork();
                    if (pid == 0) { 
                        close(fd[0]);
                        dup2(in, STDIN_FILENO);
                        if (i < p_count - 1) {
                            dup2(fd[1], STDOUT_FILENO);
                        }
                    execvp(pipes[i][0], pipes[i]);

                    // If execvp() returns, there was an error
                    perror(pipes[i][0]);
                    exit(EXIT_FAILURE);
                } else if (pid > 0) { // Parent process
                    close(fd[1]);
                    if (i < p_count - 1) {
                        in = fd[0];
                    } else {
                        // Wait for last child to finish
                        waitpid(pid, &status, 0);
                        if (WIFEXITED(status)) {
                            printf("exited with status %d\n", WEXITSTATUS(status));
                        }
                    }
                } else {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
            }
        } else { 
            // Inside the else part, if the pipe is not present, 
            // or just just one command is present
            // Come to else part and be executed. 
            pid_t pid = fork();
            if (pid == 0) { 
                execvp(args[0], args);
                perror(args[0]);
                exit(EXIT_FAILURE);
            } else if (pid > 0) { 
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    printf("exited with status %d\n", WEXITSTATUS(status));
                }
            } else {
                perror("fork");
                exit(EXIT_FAILURE);
            }
        }
        
        //If the input contains both the wildcards and pipe function will be executed here:
        if(has_wildcard && has_pipe){
            pipe(fd);
            pid_t pid = fork(); 
            if (pid < 0) {
                perror("fork() error");
                continue;
            } else if (pid == 0) { 
                //Child 
                close(fd[0]); 
                dup2(fd[1], STDOUT_FILENO); 
                close(fd[1]); 
                execl("/bin/sh", "/bin/sh", "-c", input, NULL);
                perror("execl() error"); 
                exit(EXIT_FAILURE);
            } else {
                // Parent 
                close(fd[1]); 
                waitpid(pid, &status, 0); 
                char buffer[1024];
                ssize_t n = read(fd[0], buffer, sizeof(buffer)); 
                if (n > 0) {
                    printf("%.*s", (int)n, buffer); 
                }
                close(fd[0]); 
            }
        }
    } // While
} //Interactive Mode


void batch_mode(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen() error");
        return;
    }

    char input[256];  
    char* args[MAX_ARGS];
    char* pipes[MAX_PIPES][MAX_ARGS];
    char* ptr;
    int fd[2];
    int p_count = 0;
    int arg_count = 0;
    int status;
    pid_t pid;
    printf("Welcome to Batch Mode: \n");
    while (fgets(input, sizeof(input), fp)) {

        input[strcspn(input, "\n")] = 0;
        // Arg count, total no of arg passed by the user
        arg_count = 0;
        ptr = strtok(input, " ");
        while (ptr != NULL) {
            args[arg_count++] = ptr;
            ptr = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

            // Changing directory us cd
            // If user inputs only cd will be directed to home page
            if (strcmp(args[0], "cd") == 0) {
                if (chdir(args[1]) != 0) {
                    perror("chdir() error");
                }
                continue;
            }   

            // Check working directory using pwd
            if(strcmp(args[0], "pwd") == 0){
                handle_pwd();
                continue;
            }

            // Exit the shell command type 'Exit', exited with the message of GoodBye!!
            if (strcmp(args[0], "Exit") == 0) {
                printf("Goodbye !!\n");
                exit(EXIT_SUCCESS);
                waitpid(pid, &status, 0);
            }

            // Check the user input contains any pipe '|', if contains then handle pipe
            int has_pipe = 0;
            for (int i = 0; i < arg_count; i++) {
                if (strcmp(args[i], "|") == 0) {
                    has_pipe = 1;
                    break;
                }
            }

            // Check the user input contains any pipe '*', if contains then handle Wildcard
            int has_wildcard = 0;
            for (int i = 0; i < arg_count; i++) {
                if (strchr(args[i], '*')) {
                    has_wildcard = 1;
                    break;
                }
            }

            // If the input contains the "*", if statement will run.
            if (has_wildcard) {
                int p_ind = 0;
                while (p_ind < p_count) {
                    glob_t ans;
                    memset(&ans, 0, sizeof(ans));
                    int flags = 0;
                    int a_ind = 0;
                    while (pipes[p_ind][a_ind] != NULL) {
                        glob(pipes[p_ind][a_ind], flags, NULL, &ans);
                        a_ind++;
                    }
                    a_ind = 0;
                    for (int i = 0; i < ans.gl_pathc && a_ind < MAX_ARGS-1; i++) {
                        pipes[p_ind][a_ind++] = ans.gl_pathv[i];
                    }
                    pipes[p_ind][a_ind] = NULL;
                    globfree(&ans);
                    p_ind++;
                }
            }


            // Checking how many pipe command are there, if there are more than 1 than it will split into
            // different commands and executes acordningly
            if (has_pipe) {
                // making an counter of hwo many pipe presents p_count
                p_count = 0;
                // Checking at what location pipe is presents with i_ind
                int a_ind = 0; 
                // running the while loop
                while (args[a_ind] != NULL && p_count < MAX_PIPES) {
                    int pipe_arg_count = 0;
                    while (args[a_ind] != NULL && strcmp(args[a_ind], "|") != 0
                        && pipe_arg_count < MAX_ARGS-1) {
                        pipes[p_count][pipe_arg_count++] = args[a_ind++];
                    }
                    pipes[p_count][pipe_arg_count] = NULL;
                    if (args[a_ind] != NULL && strcmp(args[a_ind], "|") == 0) {
                        a_ind++;
                    }
                    p_count++;
                }

                // Executing pipes
                int fd[2];
                int in = STDIN_FILENO;
                for (int i = 0; i < p_count; i++) {
                    pipe(fd);
                    pid_t pid = fork();
                    if (pid == 0) { 
                        close(fd[0]);
                        dup2(in, STDIN_FILENO);
                        if (i < p_count - 1) {
                            dup2(fd[1], STDOUT_FILENO);
                        }
                    execvp(pipes[i][0], pipes[i]);

                    // If execvp() returns, there was an error
                    perror(pipes[i][0]);
                    exit(EXIT_FAILURE);
                } else if (pid > 0) { // Parent process
                    close(fd[1]);
                    if (i < p_count - 1) {
                        in = fd[0];
                    } else {
                        // Wait for last child to finish
                        waitpid(pid, &status, 0);
                        if (WIFEXITED(status)) {
                            printf("exited with status %d\n", WEXITSTATUS(status));
                        }
                    }
                } else {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
            }
        } else { 
            // Inside the else part, if the pipe is not present, 
            // or just just one command is present
            // Come to else part and be executed. 
            pid_t pid = fork();
            if (pid == 0) { 
                execvp(args[0], args);
                perror(args[0]);
                exit(EXIT_FAILURE);
            } else if (pid > 0) { 
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    printf("exited with status %d\n", WEXITSTATUS(status));
                }
            } else {
                perror("fork");
                exit(EXIT_FAILURE);
            }
        }
        
        //If the input contains both the wildcards and pipe function will be executed here:
        if(has_wildcard && has_pipe){
            pipe(fd);
            pid_t pid = fork(); 
            if (pid < 0) {
                perror("fork() error");
                continue;
            } else if (pid == 0) { 
                //Child 
                close(fd[0]); 
                dup2(fd[1], STDOUT_FILENO); 
                close(fd[1]); 
                execl("/bin/sh", "/bin/sh", "-c", input, NULL);
                perror("execl() error"); 
                exit(EXIT_FAILURE);
            } else {
                // Parent 
                close(fd[1]); 
                waitpid(pid, &status, 0); 
                char buffer[1024];
                ssize_t n = read(fd[0], buffer, sizeof(buffer)); 
                if (n > 0) {
                    printf("%.*s", (int)n, buffer); 
                }
                close(fd[0]); 
            }
        }        
 
    } // While

    printf("Batch Mode Run Successfully!! \n");

} // Batch Mode



// Main
int main(int argc, char **argv) {
    // When there will be only one argument it runs into interactive mode
    if (argc == 1) {
        interactive_mode();
    } 
    // If there is two argument it runs into the batch mode
    else if (argc == 2) {
        batch_mode(argv[1]);
    } else {
        printf("Usage: %s [filename]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    return 0;
} //Main
