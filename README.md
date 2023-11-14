# traps

## xv6-book

Trap的用户/内核态切换,发生在:
- 程序执行系统调用
- 程序发生execption(page fault,除0)
- 设备触发中断

### user->kernel流程

用户跳转内核的流程
1. 保存32个寄存器
2. 保存PC寄存器
3. user mode -> supervisor mode
4. SATP由用户页表改为内核页表
5. 堆栈寄存器指向内核的一个地址(才可调用C)
6. 跳转C代码

xv6跳转内核的代码流程
1. ecall
2. uservec -- trampoline.S
3. usertrap() -- trap.c
4. syscall() --  系统调用/中断/execption
5. usertrapret() -- trap.c
6. userret()    -- trampoline.S

xv6跳转内核具体流程
1. 保存用户寄存器
   - 内核将trapframe page映射到了每个user page table
   - 内核会将trapframe page的地址保存在sscratch
2. 从trapfram中初始化SP指向内核栈
3. 从trapfram中记录当前CPU核编号(hart)
4. 从trapfram中获取内核页表，切换page table
5. 从trapfram中usertarp地址跳转usertrap()中

ecall的作用
- 保存PC寄存器的值到SEPC寄存器
- user mode -> supervisor mode
- ecall 会跳转到STVEC寄存器所指的地方(uservec)

```
在内核返回到用户空间的最后的C函数(usertrapret)中，会传递trapfram地址
一台机器总是从内核开始运行的，当机器启动的时候，它就是在内核中。 
任何时候，不管是进程第一次启动还是从一个系统调用返回，进入到用户空间的唯一方法是就是执行sret指令
在任何用户代码执行之前，内核会执行fn函数，并设置好所有的东西，例如SSCRATCH，STVEC寄存器
```

## 主要寄存器
- SATP      ->  指向page table的物理内存地址
- PC        ->  程序寄存器
- SP        ->  Stack Pointer寄存器
- SEPC      ->  保存程序计数器的值
- STVEC     ->  指向了内核中处理trap的指令的起始地址
- SSTATUS   ->  SIE 控制设备中断,SPP 控制 sret 返回的模式。
- SSCRATCH  ->  指向trapframe page(保存用户32个寄存器)

## lab-4.1

回答一些问题

## lab-4.2

主要是介绍了fp指针指开头，接下来是返回地址以及,指向上一个函数的指针\
sp指针指最低位\
给定堆栈的所有堆栈帧都位于同一页面上

## lab-4.3

gdb还是很重要的，layout命令一定要用好\
主要思想是：在trap中判断中断，然后处理中断，保存trapframe，返回用户态