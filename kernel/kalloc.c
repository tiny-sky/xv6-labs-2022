// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  struct run *freelist;
} kmems[NCPU];

void
kinit()
{
  // initlock(&kmem.lock, "kmem");
  // freerange(end, (void*)PHYSTOP);
  for (int i = 0; i < NCPU;i++){
      char buf[5];
      snprintf(buf, sizeof(buf), "kmem%d", i);
      initlock(&kmems[i].lock, buf);
  }
   freerange(end, (void *)PHYSTOP);
}

void
kfreeinit(void *pa,int cpu)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
      panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmems[cpu].lock);
  r->next = kmems[cpu].freelist;
  kmems[cpu].freelist = r;
  release(&kmems[cpu].lock);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  push_off();
  int cpu = cpuid();
  pop_off();
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
      kfreeinit(p, cpu);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
      panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  // acquire(&kmem.lock);
  // r->next = kmem.freelist;
  // kmem.freelist = r;
  // release(&kmem.lock);

  push_off();
  int cpu = cpuid();
  pop_off();
  acquire(&kmems[cpu].lock);
  r->next = kmems[cpu].freelist;
  kmems[cpu].freelist = r;
  release(&kmems[cpu].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r = 0;

  // acquire(&kmem.lock);
  // r = kmem.freelist;
  // if(r)
  //   kmem.freelist = r->next;
  // release(&kmem.lock);

  // if(r)
  //   memset((char*)r, 5, PGSIZE); // fill with junk
  // return (void*)r;

  push_off();
  int cpu = cpuid();
  pop_off();

  acquire(&kmems[cpu].lock);
  r = kmems[cpu].freelist;
  if (r) {
    kmems[cpu].freelist = r->next;
    release(&kmems[cpu].lock);
  } else {
    release(&kmems[cpu].lock);
    for (int i = 0; i < NCPU; i++) {
      if (i == cpu)
        continue;
      acquire(&kmems[i].lock);
      r = kmems[i].freelist;
      if (r) {
        kmems[i].freelist = r->next;
        release(&kmems[i].lock);
        break;
      }
      release(&kmems[i].lock);
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE);
  
  // if(r)
  //     printf("NOT NULL\n");
  // else
  //     printf("NULL\n");
  return (void*)r;
}