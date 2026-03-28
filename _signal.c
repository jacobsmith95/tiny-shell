#define _POSIX_C_SOURCE 200809
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char *arv[]) {
    kill(getpid(), argc > 1 ? atoi(argv[1]) : 0);
    sigset_t s;
    sigemptyset(&s);
    sigsuspend(&s);
    return 0;
}