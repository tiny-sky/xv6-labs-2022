#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

// echo hello too | xargs echo bye
//find . b | xargs grep hello
 int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(2, "argc too few\n");
        exit(0);
    }

    int i;
    char buf[32][12], temp;
    char* New_argv[MAXARG];

    New_argv[0] = argv[1];
    for (i = 1; i < argc - 1; i++) {
        New_argv[i]=argv[i + 1];
    }
    char* ptr = buf[i];
    while (read(0, &temp, 1) > 0) {
        if(temp == '\n'){
            *ptr = '\0';
            New_argv[i] = buf[i];
            //printf("%s\n", buf[i]);
            ptr = buf[++i];
        } else {
            *ptr++ = temp;
        }
    }
    if (fork() == 0) {
        exec(New_argv[0], New_argv);
    }
    wait(0);
    exit(0);
}  

