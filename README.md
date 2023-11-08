# pgtbl

## xv6-book

指令使用虚拟地址，机器的RAM（内存）使用物理地址，RISC-V页表硬件用来建立二者联系

xv6的虚拟地址只使用64位中的低39位
- 低39位中的低12位用于表示页内偏移（$2^{12}$=4096,从0开始）
- 低29位中的27位用于索引找到页表条目（Page Table Entries/`PTE`）
- PTE包含44位的物理页码(Physical Page Number/`PPN`)和一些标志位
- 前25位不用于转换，用于可拓展

实际的页表采用三级`树形`结构存储在物理内存中
- 在低29位zhongde27位分成3组，分别是根页表页，第二级页表页，最终的PTE
- 根页表页的物理地址存储在satp寄存器中（每一个CPU都有自己的stap）
- 前2级页表页($2^{9}$=512)的PTE存储下级页表页的起始地址
- 一级页表共需要($2^{39}$=512G)

个人理解，为什么三级页表更节约内存
- 当某一个根页表不存在时，其对应的下一级与最终的页表不用分配内存
- 所有的页表并不是都常驻内存
- 结合了TLB（高速缓存）
- 单级页表：必须是连续的，因为页表相当于是 PTE 的数组，而 PTE 在页表这个数组中的索引 index 就保存在虚拟内存地址中，内核通过页表的起始地址加上这个索引 index 才能定位到虚拟内存页对应的 PTE，近而通过 PTE 定位到映射的物理内存页。

内核的虚拟地址采用直接映射的方法到物理地址\
用户程序的虚拟地址通常被操作系统的内存管理单元（MMU）映射到物理地址
- trampoline page 映射了两次
- 保护页(guard page)的`PTE`的(PTE_V 没有设置)

## 主要函数
- memlayout.h -> 记录了内核，I/O设备，boot等的起始地址
- vm.c        -> 操作地址空间与页表相关的操作代码
  1. walk       -> 为虚拟地址寻找PTE（设置alloc参数，会分配新页表）
  2. mappages   -> 装载找到的PTE
   
  3. kvinit     -> 初始化内核页表
  4. kvmake     -> 创建内核页表
  5. kvmap      -> 装载内核页表
  6. kvminithart-> 将根页表页写入寄存器satp，并刷新TLB
- trampoline.S -> 从内核态返回用户态
  1. sfence.vma ->  刷新TLB
- kalloc.c      -> 分配的物理内存页
  1. kinit      -> 初始化全部可用物理地址
  2. freerange  -> 初始化两地址之间的物理地址
  3. kfree      -> 用`1`格式化这个页面,头插到freelist
  4. kalloc     -> 用`5`填充表明可用,从freelist拿下

## 高级用法
```c
struct run {
  struct run *next;
};
```
上面的结构体巧妙的用到了指针地址与值的关系\
ptr = 0x1234\
(struct run *)ptr.run就可用于下一个值

```c
#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
```
上面的代码展示了一个变量如何向上以4096对齐

## lab-3.1

- 宏定义`LAB_PGTBL`在makefile中,只有在执行的时候会被检测到
- 每一个页都需要先进行页面分配(kalloc) -> 然后再装载映射(mappages) -> 在进程释放之后,需要把分配的页面释放(uvmunmap)

## lab-3.2

- 仿写freewalk,遍历页表,判断权限位
- DFS递归打印树的深度,所以需要一个传入depth的递归函数

## lab-3.3

- 通过argaddr接受参数,copyout写回参数
- vx6的PTE_A参数位是由硬件所设置的
```
The RISC-V hardware page walker marks these bits in the PTE whenever it resolves a TLB miss
```
- 总之就是获取每一页的PTE然后检测其标志位
