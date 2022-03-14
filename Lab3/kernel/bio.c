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
#include <stddef.h>
#define NBUCKETS 13
#define hash(x) (x) % NBUCKETS
struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKETS];

  // struct buf free_head;
  // struct spinlock free_lock;
} bcache;


void
binit(void)
{
  struct buf *b;
  int offset = 0;
  // 初始化NBUCKETS个hash桶的锁
  for(int i = 0; i < NBUCKETS; i++)
  {
    char* lock_name = "bcache";
    initlock(&bcache.lock[i], lock_name);
      // Create linked list of buffers
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
    // 将块分配给hash桶
    int j = 0;
    int block_per_bucket = NBUF/NBUCKETS;
    if(i==NBUCKETS-1)
    {
      while(offset < NBUF)
      {
        b = bcache.buf+offset;
        b->next = bcache.head[i].next;
        b->prev = &bcache.head[i];
        initsleeplock(&b->lock, "buffer");
        bcache.head[i].next->prev = b;
        bcache.head[i].next = b;
        b->blockno = i;
        offset++;
      }
    }
    else
    {
      while(j<block_per_bucket)
      {
        b = bcache.buf+offset;
        b->next = bcache.head[i].next;
        b->prev = &bcache.head[i];
        initsleeplock(&b->lock, "buffer");
        bcache.head[i].next->prev = b;
        bcache.head[i].next = b;
        b->blockno = i;
        offset++;
        j++;
      }
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int no = hash(blockno);
  // printf("no:%d\n", no);
  acquire(&bcache.lock[no]);

  // Is the block already cached?
  for(b = bcache.head[no].next; b != &bcache.head[no]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[no]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 首先尝试在自己的hash桶中寻找空闲块
  for(b = bcache.head[no].next; b != &bcache.head[no]; b = b->next){
    if(b->refcnt==0){
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[no]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 没有命中，到其他hash桶寻找
  int i = (no+1) % NBUCKETS;
  while(i != no)
  {
    acquire(&bcache.lock[i]);
    for(b = bcache.head[i].next; b != &bcache.head[i]; b = b->next){
      if(b->refcnt == 0) {
        // 给找到的空闲块赋值
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        // 加入对应的hash桶
        b->prev->next = b->next;
        b->next->prev = b->prev;
        // acquire(&bcache.lock[no]);
        b->next = bcache.head[no].next;
        b->prev = &bcache.head[no];
        bcache.head[no].next->prev = b;
        bcache.head[no].next = b;
        release(&bcache.lock[i]);
        release(&bcache.lock[no]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]);
    i = (i+1) % NBUCKETS;
  }
  release(&bcache.lock[no]);
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

  int no = hash(b->blockno);
  acquire(&bcache.lock[no]);
  b->refcnt--;
  release(&bcache.lock[no]);
}

void
bpin(struct buf *b) {
  int no = hash(b->blockno);
  acquire(&bcache.lock[no]);
  b->refcnt++;
  release(&bcache.lock[no]);
}

void
bunpin(struct buf *b) {
  int no = hash(b->blockno);
  acquire(&bcache.lock[no]);
  b->refcnt--;
  release(&bcache.lock[no]);
}


