// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define HASHNUM 13
#define HASH(key) (key % HASHNUM);

struct HashTable{
  struct spinlock lock;
  struct buf buf;
} HashTable[HASHNUM];

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf head;
} bcache;

void
binit(void)
{
    struct buf* b;
    int hash = 0;
    initlock(&bcache.lock, "bcache");

// Create linked list of buffers
/*     bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;
    for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        initsleeplock(&b->lock, "buffer");
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }   */

    for (int i = 0; i < HASHNUM;i++){
        char buf[8];
        snprintf(buf, 8, "bcache%d", i);
        initlock(&HashTable[i].lock, buf);
        HashTable[i].buf.prev = &HashTable[i].buf;
        HashTable[i].buf.next = &HashTable[i].buf;
    }

        for (b = bcache.buf; b < bcache.buf + NBUF; b++, hash++) {
            int index = HASH(hash);
            b->next = HashTable[index].buf.next;
            b->prev = &HashTable[index].buf;
            initsleeplock(&b->lock, "buffer");
            HashTable[index].buf.next->prev = b;
            HashTable[index].buf.next = b;
        }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int index = blockno % HASHNUM;

  acquire(&HashTable[index].lock);
  //acquire(&bcache.lock);

    for (b = HashTable[index].buf.next; b !=&HashTable[index].buf;b = b->next){
       if(b->dev == dev && b->blockno ==blockno){
          b->refcnt++; 
          release(&HashTable[index].lock);
          acquiresleep(&b->lock);
          return b;
      }
    } 

  // Is the block already cached?
  /*  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }  
  }*/

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.

  /*  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
        if(b->refcnt == 0) {
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          release(&bcache.lock);
          acquiresleep(&b->lock);
          return b;
        }
      }  */

   for (b = HashTable[index].buf.prev; b != &HashTable[index].buf;b = b->next){
     if(b->refcnt == 0) {
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          release(&HashTable[index].lock);
          acquiresleep(&b->lock);
          return b;
        }
  }

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

          //printf("ADD:all ->%d  use -> %d :%d->%d\n",HashTable[index].num,HashTable[index].use,i,index);
          release(&HashTable[i].lock);
          release(&HashTable[index].lock);
          acquiresleep(&b->lock);
          return b;
        }
    }
    release(&HashTable[i].lock);
  }
      panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int index = b->blockno % HASHNUM;
  acquire(&HashTable[index].lock);
  b->refcnt--;
  if(b->refcnt == 0){
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = HashTable[index].buf.next;
    b->prev = &HashTable[index].buf;
    HashTable[index].buf.next->prev = b;
    HashTable[index].buf.next = b;
  }

  release(&HashTable[index].lock); 

  /* acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  release(&bcache.lock); */
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


