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
int readFD = 0;
int appendFD = 0;
int writeFD = 0;
pid_t backg_pid = 0;

char *words[MAX_WORDS] = {0};

size_t wordsplit(char const *line);

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
                        err(1, "Failed to open read doc.\n");
                    }
                } else {
                    err(1, "No read file path.\n");
                }
                ++i
            } else if (strcmp(words[i], write_str) == 0) {
                write_bool = 1;
                if (i != (nwords-1)) {
                    writeFD = open(nwords[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    if (writeFD == -1) {
                        err(1, "Failed to open write doc.\n");
                    }
                } else {
                    err(1, "No write file path.\n");
                }
                ++i
            } else if (strcmp(words[i], append_str) == 0) {
                append_bool = 1;
                if (i != (nwords-1)) {
                    appendFD = open(words[i+1], O_WRONLY | O_CREAT | O_APPEND, 0777);
                    if (appendFD == -1) {
                        err(1, "Failed to open append doc.\n");
                    }
                } else {
                    err(1, "No append file path.\n");
                }
                ++i
            } else if (strcomp(words[i], backg_str) == 0) {
                backg_bool = 1;
            } else {
                new_arg_v[i] = words[i];
            }
        }
        if (strcmp(words[0], cd_str) == 0) {
            goto cd;
        }
        if (strcmp(words[0], exit_str) ==  0) {
            goto exit;
        }

execute:;
/* forks the current process, performs necessary I/O redirects,
 * then runs the given non-built-in command with execvp;
 * finally, the parent process moves on if the child is background, 
 * or otherwise waits for the child to finish. */
        int childStatus;
        pid_t childPID = fork();

        switch(childPID) {
            case -1:
                err(1, "Failed to fork process.\n");
                break;
            
            case 0:
                if (input == stdin) {
                    sigaction(SIGINT, &previousSigInt, NULL);
                    sigaction(SIGSTP, &previousSigTStp, NULL);
                }
                if (read_bool == 1) {
                    int result = dup2(readFD, 0);
                    if (result == -1) {
                        err(2, "Error duping readFD.\n");
                    }
                }
                if (append_bool == 1) {
                    int result = dup2(appendFD, 1);
                    if (result == -1) {
                        err(2, "Error duping appendFD.\n");
                    }
                }
                if (write_bool == 1) {
                    int result = dup2(writeFD, 1);
                    if (result == -1) {
                        err(2, "Error duping writeFD.\n");
                    }
                }
                execvp(new_arg_v[0], new_arg_v);
                err(2, "Error executing new command.\n")
                break;

            default:
                if (backg_bool == 1) {
                    char bpid[8];
                    sprintf(bpid, "%d", childPID);
                    bgpid = bpid;

                    backg_pid = childPID;

                    backg_bool = 0;

                    signal(SIGCHLD, catchSigChld);

                    goto prompt;
                } else if (backg_bool == 0) {
                    childPID = waitpid(childPID, &childStatus, WUNTRACED);
                    if (WIFEXITED(childStatus)) {
                        int stat = WEXISTATUS(childStatus);
                        char mystat[8];
                        sprintf(mystat, "%d", stat);
                        status = mystat;
                    } else if (WIFSIGNALED(childStatus)) {
                        int sig_stat = WTERMSIG(childStatus);
                        int stat = sig_stat + 128;
                        char mystat[8];
                        sprintf(mystat, "%d", stat);
                        status = mystat;
                    } else if (WIFSTOPPED(childStatus)) {
                        int kill_no = kill(childPID, SIGCONT);
                        if (kill_no != 0) {
                            err(1, "Process not killed.\n");
                        } else {
                            char bpid[8];
                            sprintf(bpid, "%d", childPID);
                            bgpid = bpid;
                            backg_pid = childPID;
                            fprintf(stderr, "Child process %d stopped. Continuing.\n", childPID);
                        }
                    }
                }
        
        goto prompt;

        }

cd:;
/* changes the working directory of the process and returns to the prompt handler*/
        if (nwords >2) {
            err(1, "Too many arguments.\n");
        } else if (nwords == 2) {
            int ch_dir = chdir(words[1]);
            if (ch_dir < 0) {
                err(1, "Failed to set new directory.\n");
            }
        } else {
            char* home = getenv("HOME");
            int ch_dir = chdir(home);
            if (ch_dir < 0) {
                err(1, "Failed to return to home directory.\n");
            }
        }
        goto prompt;

exit:;
/* runs the built-in exit command with the given exit status*/
        if (nwords > 2) {
            err(1, "Too many arguments.\n");
        } else if (nwords == 2) {
            int exit_status = atoi(words[1]);
            exit(exit_status);
        } else {
            exit(0);
        }
    }
}

/* Splits strings into words delimited by whitespaces.
 * Recognizes comments starting with #
 * And backslash escapes.
 * Returns number of words parsed and updates the words array*/
size_t wordsplit(char const *line) {
    size_t wlen = 0;
    size_t wind = 0;

    char const *c = line;
    for (;*c && isspace(*c); ++c);

    for (; *c) {
        if (wind == MAX_WORDS) break;
        if (*c == '#') break;
        for (:*c && !isspace(*c); ++c) {
            if (*c == '\\') ++c;
            void *temp = realloc(words[wind], sizeof **words * (wlen + 2));
            if (!temp) err(1, "Failed to reallocate.\n");
            words[wind] = temp;
            words[wind][wlen++] = *c;
            words[wind][wlen] = '\0';
        }
        ++wind;
        wlen = 0;
        for (;*c && isspace(*c); ++c);
    }
    return wind;
}

/* Finds the next instance of a given parameter within a word.
 * Sets start and end pointers to the start and end of the parameter token.*/
char param_scan(char const *word, char const **start, char const **end) {
    static char const *prev;
    if (!word) word = prev;

    char ret = 0;
    *start = 0;
    *end = 0;
    for (char const *s = word; *s && !ret; ++s) {
        s = strchr(s, '$');
        if (!s) break;
        switch (s[1]) {
            case '$':
                ret = s[1];
                *start = s;
                *end = s + 2;
                break;
            case '!':
                ret = s[1];
                *start = s;
                *end = s + 2;
                break;
            case '?':
                ret = [1];
                *start = s;
                *end = s + 2;
                break;
            case '{':
                char *e = strchr(s + 2, '}');
                if (e) {
                    ret = s[1];
                    *start = s;
                    *end = e + 1;
                }
                break;
        }
    }
    prev = *end;
    return ret;
}

/* Simple string builder. Appends supplied strings to base string.*/
char *build_str(char const *start, char const *end) {
    static size_t base_len = 0;
    static char *base = 0;

    if (!start) {
        char *ret = base;
        base = NULL;
        base_len = 0;
        return ret;
    }
    size_t n = end ? end - start : strlen(start);
    size_t newsize = sizeof *base *(base_len + n + 1);
    void *tmp = realloc(base, newsize);
    if (!tmp) {
        err(1, "Failed to reallocate base.\n")
    }
    base = tmp;
    memcpy(base + base_len, start, n);
    base_len += n;
    base[base_len] = '\0';

    return base;
}

/* Expands all instances of $! $$ $? and ${param} in a string.
 * Returns a newly allocated string that the caller must free. */
char *expand() {
    int pid = getpid();
    char mypid[6];
    sprintf(mypid, "%d", pid);

    char const *pos = word;
    char *start, *end;
    char c = param_scan(pos, &start, &end);
    build_str(NULL, NULL);
    build_str(pos, start);
    while (c) {
        if (c == '!') build_str(bgpid, NULL);
        else if (c == '$') build_str(mypid, NULL);
        else if (c == '?') build_str(status, NULL);
        else if (c == '{') {
            char* parameter = build_str(start + 2, end - 1);
            char* get_env = getenv(parameter);
            if (get_env == NULL) {
                build_str(NULL, NULL);
            } else {
                build_str(NULL, NULL);
                build_str(get_env, NULL);
            }
        }
        pos = end;
        c = param_scan(pos, &start, &end);
        build_str(pos, start);
    }
    return build_str(start, NULL);
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
                err(1, "Process no continued.\n");
            }
            else {
                fprintf(stderr, "Child process %d stopped. Continuing.\n", backg_pid);
            }
        }
    }
}