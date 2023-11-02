#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
    int fd[2];
    int pid;

    if (pipe(fd) < 0) {
        fprintf(2, "pipe failed!\n");
        exit(1);
    }
    if ((pid = fork()) < 0) {
        fprintf(2, "fork failed!\n");
        exit(1);
    }
    if (pid == 0) {
        char* buf;
        read(fd[1], &buf, 1);
        printf("%d: received ping\n", getpid());
        write(fd[1], &buf, 1);
        close(fd[0]);
        close(fd[1]);
        exit(0);
    }
    char data = 'Y';
    write(fd[0], &data, 1);
    read(fd[0], &data, 1);
    printf("%d: received pong\n", getpid());
    close(fd[1]);
    close(fd[0]);
    exit(0);
}
