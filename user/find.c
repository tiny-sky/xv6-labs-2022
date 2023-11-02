#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char* path, char* file_name) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        if (de.inum == 0) {
            continue;
        }
        if (!strcmp(de.name, ".") || !strcmp(de.name, "..")) {
            continue;
        }

        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (stat(buf, &st) < 0) {
            printf("find: cannot stat %s\n", buf);
            continue;
        }

        switch (st.type) {
            case T_FILE: {
                if (strcmp(de.name, file_name) == 0) {
                    printf("%s\n", buf);
                }
                break;
            }
            case T_DIR: {
                find(buf, file_name);
                break;
            }
        }
    }
    close(fd);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("argv too few\n");
        exit(0);
    }

    char* path_name = argv[1];
    char* file_name = argv[2];
    find(path_name, file_name);
    exit(0);
}
