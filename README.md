# COW

## xv6-book

copy on write fork 即在fork的时候并没有分配物理页\
而是仅仅将子进程与父进程共用页表\
只有当对这个页面进行写操作的时候\
通过引发Page Fault,引发usertrap，真正分配物理页

## 遇到的一些问题

- 由于对全局计数的引入，故需要加入锁机制
```c
struct PAGES{
    #define INDEX ((PHYSTOP-KERNBASE) / PGSIZE)
    struct spinlock lock;
    char pages[INDEX];
} Pages;
```
- 对于子映射到父PTE时，应判断是否有写权限，才给予COW位
```c
if (*pte & PTE_W) { 
      *pte ^= PTE_W; 
      *pte |= PTE_COW; 
    }
```
- walk返回的就是最底层指向物理地址的PTE\
而mappages则通过传入的新页表与新页表索引获取到底层PTE\
后将物理地址与权限位写入到新的PTE中
```c
*pte = PA2PTE(pa) | perm | PTE_V;
```
- 由于在freerang会先调用kfree,而kfree中会将索引值减一，所以需要在freerange中加一
```c
for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    kfree(p);
    Pages_set((uint64)p, 1);
  }
```
- 在usertrap中，检测Page Fault ，并获取缺页项
```c
if(r_scause() == 15){

    uint64 va = r_stval();
    if(uvmforkcopy(p->pagetable,va) < 0)
        p->killed = 1;
  }
```