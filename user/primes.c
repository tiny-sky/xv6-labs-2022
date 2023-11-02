#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

__attribute__((noreturn))
void child(int* pipes) {
    int Curdata;
    int chpipes[2];
    close(pipes[1]);

    int ret = read(pipes[0], &Curdata, 1);
    if (!ret)
        exit(0);

    if (pipe(chpipes) < 0) {
        fprintf(2, "pipe failed!\n");
        exit(1);
    }

    if (!fork()) {
        child(chpipes);
    } else {
        int buf;
        close(chpipes[0]);
        printf("prime %d\n", Curdata);
        while (read(pipes[0], &buf, 1) > 0) {
            if (buf % Curdata) {
                write(chpipes[1], &buf, 1);
            }
        }
        close(chpipes[1]);
        wait(0);
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    int fd[2], pid;

    if (pipe(fd) < 0) {
        fprintf(2, "pipe failed!\n");
        exit(1);
    }
    if ((pid = fork()) < 0) {
        fprintf(2, "fork failed!\n");
        exit(1);
    }
    if (pid == 0) {
        child(fd);
    }
    close(fd[0]);
    for (int i = 2; i <= 35; i++) {
        write(fd[1], &i, 1);
    }
    close(fd[1]);
    wait(&pid);
    exit(0);
}