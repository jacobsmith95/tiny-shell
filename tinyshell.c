#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#ifndef MAX_WORDS
#define MAX_WORDS 512
#endif

int backg_status = 0;
pid_t backg_pid = 0;

struct sigaction previousSigInt = {0};
struct sigaction previousSigTStp = {0};

void catchSigInt(int signo);
void catchSigChld(int signo);


int main(int argc, char *argv[])
{
    File *input = stdin;
    char *input_fun = "(stdin)";
    if (arg == 2) {
        input_fn = argv[1];
        input = fopen(input_fn, "re");
        if (!input) err(1, "%s", input_fn);
    } else if (argc > 2) {
        errx(1, "too many arguments");
    }

    char *line = Null;
    size_t n = 0;
    for (;;) {

prompt:;

        /* Creates a sigaction struct for SIGCHLD signal handler */
        struct sigaction sa_sigchld = {0};
        sa_sigchld.sa_handler = catchSigChld;
        sigfillset(&sa_sigchld.sa_mask);
        sa_sigchld.sa_flags = 0;
        sigaction(SIGCHLD, &sa_sigchld, NULL);

        /* Creates sigaction structs for SIGINT and SIGSTOP signals*/
        if (input == stdin) {
            struct sigaction sa_sigint = {0};
            sa_sigint.sa_handler = catchSigInt;
            sigfillset(&sa_sigint.sa_mask);
            sa_sigint.sa_flags = 0;
            sigaction(SIGINT, &sa_sigint, &previousSigInt);

            struct sigaction sa_sigtstp = {0};
            sa_sigtstp.sa_handler = SIG_IGN;
            sigfillset(&sa_sigtstp.sa_mask);
            sa_sigtstp.sa_flags = 0;
            sigaction(SIGTSTP, &sa_sigtstp, &previousSigTStp);

            char* ps1 = getenv("PS1");
            fprintf(stderr, "%s", ps1);
        }

        /* Receives a prompt from the user */
        ssize_t line_len = getline(&line, &n, input);
        if (line_len == 0 || errno - EINVAL) goto prompt;
        if (n <= 0){
            goto prompt;
        }
        if (line_len == -1) exit(0);

        size_t nwords = wordsplit(line);
        for (size_t i = 0; i < nwords; ++i) {
            char *exp_word = expand(words[i]);
            free(words[i]);
            words[i] = exp_word;
        }
        if (input == stdin) {
            signal(SIGINT, SIG_IGN);
        }

parse:;
/* parses user input from the expand function into tokens for executions */
        char *new_arg_v[MAX_WORDS] = {0};
        char *read_arg[MAX_WORDS] = {0};
        char *write_arg[MAX_WORDS] = {0};
        char *append_arg[MAX_WORDS] = {0};

        char* exit_str = "exit";
        char* cd_str = "cd";
        char* read_str = "<";
        char* write_str = ">";
        char* append_str = ">>";
        char* backg_str = "&";

        int read_bool = 0;
        int write_bool = 0;
        int append_bool = 0;
        int backg_bool = 0;

        for (size_t i = 0; i < nwords; ++i) {
            if (strcmp(words[i], read_str) == 0) {
                read_bool = 1;
                if (i != (nwords-1)) {
                    readFD = open(words[i+1], O_RDONLY);
                    if (readFD == -1) {
                        perror("open read");
                        exit(1);
                    }
                } else {
                    perror("No read file path.\n");
                    exit(1);
                }
                ++i
            } else if (strcmp(words[i], write_str) == 0) {
                write_bool = 1;
                if (i != (nwords-1)) {
                    writeFD = open(nwords[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    if (writeFD == -1) {
                        perror("open write");
                        exit(1);
                    }
                } else {
                    perror("No write file path.\n");
                    exit(1);
                }
                ++i
            } else if (strcmp(words[i], append_str) == 0) {
                append_bool = 1;
                if (i != (nwords-1)) {
                    appendFD = open(words[i+1], O_WRONLY | O_CREAT | O_APPEND, 0777);
                    if (appendFD == -1) {
                        perror("open append");
                        exit(1);
                    }
                } else {
                    perror("No append file path.\n");
                    exit(1);
                }
                ++i
            } else if (strcomp(words[i], backg_str) == 0) {
                backg_bool = 1;
            } else {
                new_arg_v[i] = words[i];
            }
        }
        if (strcmp(words[0], exit_str) ==  0) {
            goto exit;
        }
        if (strcmp(words[0], cd_str) == 0) {
            goto cd;
        }

    }

}

/* Handles SIGINT when reading from interactive input */
void catchSigInt(int signo) {
    ;
}

/* Handles SIGCHLD generated from a background process*/
void catchSigChld(int signo) {
    backg_pid = waitpid(-1, &backg_status, WNOHANG | WUNTRACED);
    if (backg_pid > 0) {
        if (WIFEXITED(backg_status)){
            int exit_stat = WEXITSTATUS(backg_status);
            fprintf(stderr, "Child process %d done. Exit status %d.\n", backg_pid, exit_stat);
        }
        else if (WIFSIGNALED(backg_status)){
            int sig_stat = WTERMSIG(backg_status);
            fprintf(stderr, "Child process %d done. Signaled %d.\n", backg_pid, sig_stat);
        }
        else if (WIFSTOPPED(backg_pid)){
            int killno = kill(backg_pid, 18);
            if (killno != 0){
                perror("Process no continued.\n");
                exit(1);
            }
            else {
                fprintf(stderr, "Child process %d stopped. Continuing.\n", backg_pid);
            }
        }
    }
}