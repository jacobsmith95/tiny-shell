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