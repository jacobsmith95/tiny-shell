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