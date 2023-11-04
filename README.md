# util

## xv6-book

当进程需要调用内核服务时，它会调用系统调用，即操作系统接口中的调用之一。系统调用进入内核;内核执行服务并返回。因此，进程在用户空间和内核空间中交替执行。

shell 是一个普通程序，它从用户那里读取命令并执行它们。shell 是一个用户程序，而不是内核的一部分(sh.c)。

下面是系统提供给用户的可调用函数
```c 
int fork()  //Create a process, return child’s PID.
int exit(int status)    //Terminate the current process; status reported to wait(). No return.
int wait(int *status)   //Wait for a child to exit; exit status in *status; returns child PID.
int kill(int pid)   //Terminate process PID. Returns 0, or -1 for error.
int getpid()    //Return the current process’s PID.
int sleep(int n)    //Pause for n clock ticks.
int exec(char *file, char *argv[])  //Load a file and execute it with arguments; only returns if error.
char *sbrk(int n)   //Grow process’s memory by n bytes. Returns start of new memory.
int open(char *file, int flags)     //Open a file; flags indicate read/write; returns an fd (file descriptor).
int write(int fd, char *buf, int n)     //Write n bytes from buf to file descriptor fd; returns n.
int read(int fd, char *buf, int n)  //Read n bytes into buf; returns number read; or 0 if end of file.
int close(int fd)   //Release open file fd.
int dup(int fd)     //Return a new file descriptor referring to the same file as fd.
int pipe(int p[])   //Create a pipe, put read/write file descriptors in p[0] and p[1].
int chdir(char *dir)    //Change the current directory.
int mkdir(char *dir)    //Create a new directory.
int mknod(char *file, int, int)     //Create a device file.
int fstat(int fd, struct stat *st)  //Place info about an open file into *st.
int stat(char *file, struct stat *st)   //Place info about a named file into *st.
int link(char *file1, char *file2)  //Create another name (file2) for the file file1.
int unlink(char *file)  //Remove a file.
```

## 环境配置

1. 由于本环境使用clang编译器，所有导致一些头文件无法找到
```
  在Clangd的Arguments中添加--complie-commands-dir=${worksapceFolder}/build/${buildType}
```
2. 头文件包含的顺序很重要
```
   因为在链接中，头文件中的代码片段是直接拼接到目标代码中的，所以导致一些变量的依赖顺序问题
```
3. 在一些版本在显示runcmd的infinite recursion detected问题
```
    修改为void runcmd(struct cmd*) __attribute__((noreturn));

    __attribute__是一种编译属性优化的声明
    在该例子中表示该函数不需要返回值，一般用于未执行完就需要退出的函数
```

## lab-1.1

简单的sleep函数调用

## lab-1.2

父进程去写，子进程去读，然后层层传递数值，由于使用了递归，引入了__attribute__((noreturn))编译属性

## lab-1.3

很简单，大部分仿写ls.c

## lab-1.4

不是很难，主要考察对字符串的拼接问题，值得注意的是：
- xv6的`|`已经实现，所以`|`之前的数据已经被写到标准输入(0)中
- exec的使用在argv字符数组中，传入的第一个argv[0]是`命令`！，并不全是参数
