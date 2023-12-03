# Lock

## xv6-book

在yield中获取当前进程的锁，保证了三个操作的原子性
- 进程状态从RUNNING -> RUNABLE
- 进程的寄存器保存在context中
- 停止使用当前进程的栈，而改为内核栈

线程的切换涉及： A内核线程 -> 内核调度线程 -> B内核线程
- A将当前的context保存在proc结构体中
- 在swtch中，ra设为scheduler函数的地址，sp指向内核栈
- 运行scheduler
- 在swtch中，ra设为B线程之前调用swtch的返回地址
- 返回usertrapret中，返回用户态

##  高级用法
```c
struct entry *table[NBUCKET];
insert(key, value, &table[i], table[i]);

static void 
insert(int key, int value, struct entry **p, struct entry *n)
{
  struct entry *e = malloc(sizeof(struct entry));
  e->key = key;
  e->value = value;
  e->next = n;
  *p = e;
}
```
这个的目的实现无头节点的头插法

值得注意的是传入第一个元素的地址以及内容\
将新的节点 e 的后驱指向原本的地址\
再将原本指针的地址替换为当前新的节点的地址