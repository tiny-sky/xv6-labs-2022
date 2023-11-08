#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  int numpage;
  uint64 userbuf;
  uint64 usermask;
  unsigned int kvmask = 0;

  argint(1, &numpage);
  argaddr(0, &userbuf);
  argaddr(2, &usermask);

  if(numpage < 0 || numpage > 32) {
    return -1;
  }
  struct proc* p = myproc();
  
  for (int i = 0; i < 32; i++){
    if(userbuf > MAXVA)
        return 1;
    pte_t* pte = walk(p->pagetable, userbuf + i * PGSIZE, 0);
    if(!pte)
        panic("pagaccess: walk");
    //标记哪些页被访问过
    if (*pte & PTE_A) {
        //printf("origin Kvmask -> %d\n", kvmask);
        //printf("current -> %d\n", 1 << i);
        kvmask |= (1 << i);
        *pte &= (~PTE_A);
    }
}
//printf("\n\nkvmask->%d\n\n", kvmask);
if (copyout(p->pagetable, usermask, (char*)&kvmask, 32) < 0)
    return 1;
return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
