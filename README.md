# lock

## xv6-book

### Lock

1. 在多进程读写某些共享数据时，竞争的结果取决于进程在处理器的时间，以及内存系统的排序\
此时使用锁来串行化并发的临界区

2. xv6锁的类型 

- spinlocks（自旋锁）
- sleeplocks（睡眠锁）

3. 当锁可用的时候locked字段为0,被持有的时候为1\
在RISC-V中，使用 amoswap r,a 来保证原子操作

4. 如果执行代码需要持有多个锁，则所有代码的获取顺序必须相同

5. 当xv6自旋锁保护与中断处理程序共用的数据时可能会导致死锁

- 当自旋锁被中断处理程序使用时，CPU必须保证在启用中断时不能持有该锁
- xv6 -> 当CPU获取锁时，xv6总是禁用该CPU上的中断，当中断可出现在其他CPU上

6. acquire 和release 分别调用push_off与pop_off来跟踪当前CPU上锁的嵌套等级

7. 指令不一定时顺序执行，CPU与编译器通过内存模型来控制重新排序

8. 睡眠锁与自旋锁

- 自旋锁不会让出（yield）CPU
- 睡眠锁会对sleep的调用原子的让出CPU并释放自旋锁，其他线程就可在等待中执行

### File system

1. 文件系统的目的是组织与存储数据
   - 文件系统需要磁盘的数据结构来表示目录与文件树，记录不同的标识文件
   - 支持崩溃恢复
   - 协调保持不变量
   - 保持常用块的缓存
2. xv6的文件系统实现分为7层
3. Buffer cache有几个主要的任务
    - 保证磁盘块在内存中只有一个副本(bcache.lock)
    - 只有一个内核线程使用该副本(睡眠锁)
    - 缓存常用块
## courses

如果两组锁完全独立，则没有必要对所有的锁进行排序\
但是所有锁需要对共同使用的锁进行一些排序

## tests

### lab-8.1

在kinit的时候，给8个CPU每一个都分配一个空闲链表\
每一个空闲链表都有一个对应的睡眠锁

抢占空闲页的实现比较简单
```c
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
```
### lab-8.2

这个lab本质上也是将大锁转换为小锁的过程，通过降低锁的粒度来进行优化

- 使用哈希桶来对不同的block进行离散
```c
#define HASHNUM 13
#define HASH(key) (key % HASHNUM);

struct HashTable{
  struct spinlock lock;
  struct buf buf;
} HashTable[HASHNUM];
```
- 实现当一个桶的空闲缓存块不足时，进行对其他桶的缓存块获取\
这里的实现通过循环依次访问每一个哈希桶
```c
for (int i = 0; i < HASHNUM; i++) {
       if (i == index || HashTable[i].lock.cpu != 0)
          continue; 
      acquire(&HashTable[i].lock);
      for (b = HashTable[i].buf.prev; b != &HashTable[i].buf;b = b->next){
        if(b->refcnt == 0) {
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;

          b->next->prev = b->prev;
          b->prev->next = b->next;
          b->next = HashTable[index].buf.next;
          b->prev = &HashTable[index].buf;
          HashTable[index].buf.next->prev = b;
          HashTable[index].buf.next = b;

          release(&HashTable[i].lock);
          release(&HashTable[index].lock);
          acquiresleep(&b->lock);
          return b;
        }
    }
    release(&HashTable[i].lock);
  }
```
为了避免死锁过程，对HashTable[i].lock.cpu 进行判断，来决定获取的哈希桶

- 值得吸取的经验是：通过双向链表

  prev方向 是还未使用过的缓存块

  next方向 是已经使用的缓存块
```c
if(b->refcnt == 0){
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = HashTable[index].buf.next;
    b->prev = &HashTable[index].buf;
    HashTable[index].buf.next->prev = b;
    HashTable[index].buf.next = b;
  }
```