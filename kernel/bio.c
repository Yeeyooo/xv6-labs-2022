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

#define NBUCKS 13

struct {
  struct spinlock global_lock;
  struct spinlock lock[NBUCKS];
  struct buf buf[NBUF];
  
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKS];
} bcache;

int checklock(int id, int other) {
  int num = NBUCKS / 2;
  if (id <= num && other > id &&  other <= id + num) {
    return 0;
  }
  else if ((id < other && other < NBUCKS) || other <= (id + num) % NBUCKS) {
    return 0;
  }
  return 1;
}


void
binit(void)
{
  struct buf *b;

  initlock(&bcache.global_lock, "bcache-global");
  for (int i = 0; i < NBUCKS; i++) {
    initlock(&bcache.lock[i], "bcache");
    bcache.head[i].next = 0;
  }

  for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
    initsleeplock(&b->lock, "buffer");
  }

  // bcache.head[0].next = bcache.buf;    // put all buffers into bucket 0 initially
  // for (b = bcache.buf; b < bcache.buf + NBUF - 1; b++) {
  //   b->next = b + 1;
  //   initsleeplock(&b->lock, "buffer");
  // }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf *last;
  int index = blockno % NBUCKS;
  acquire(&bcache.lock[index]);
  for (b = bcache.head[index].next, last = &bcache.head[index]; b; b = b->next) {
    if (!b->next) {
      last = b;
    }
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.lock[index]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached
  acquire(&bcache.global_lock);
  struct buf *LRU = 0;
  for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
    if (b->refcnt == 0) {
      if (LRU == 0) {
        LRU = b;
        continue;
      }
      else if (b->tick < LRU->tick){
        LRU = b;
      }
    }
  }
  // find a buffer with 0 refcnt and least recently used
  if (LRU != 0) {
    uint prev_tick = LRU->tick;
    int prev_index = (LRU->blockno) % NBUCKS;
    if (prev_tick == 0) {
      LRU->dev = dev;
      LRU->blockno = blockno;
      LRU->valid = 0;
      LRU->refcnt = 1;
      LRU->tick = ticks;
      last->next = LRU;
      LRU->prev = last;
    }
    else if (prev_index != index) {  // previously on the other bucket
      acquire(&bcache.lock[prev_index]);
      LRU->prev->next = LRU->next;
      if (LRU->next) {
        LRU->next->prev = LRU->prev;
      }
      release(&bcache.lock[prev_index]);
      LRU->dev = dev;
      LRU->blockno = blockno;
      LRU->valid = 0;
      LRU->refcnt = 1;
      LRU->tick = ticks;
      last->next = LRU;
      LRU->prev = last;
      LRU->next = 0;
    }
    else {
      LRU->dev = dev;
      LRU->blockno = blockno;
      LRU->valid = 0;
      LRU->refcnt = 1;
      LRU->tick = ticks;
    }
    release(&bcache.global_lock);
    release(&bcache.lock[index]);
    acquiresleep(&LRU->lock);
    return LRU;
  }

  panic("bgets: no buffers");

  // struct buf *b;
  // int index = blockno % NBUCKS;   // corresponding hash index
  // acquire(&bcache.lock[index]);   // remember to release this lock!

  // for (b = bcache.head[index].next; b; b = b->next) {  // search in current bucket list
  //   if (b->dev == dev && b->blockno == blockno) {      // find a match
  //     b->refcnt++;
  //     release(&bcache.lock[index]);  
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  // // Not cached
  // // search in current bucket
  // for (b = bcache.head[index].next; b; b = b->next) {
  //   if (b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock[index]);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  // // search in other buckets
  // struct buf *selected = 0;
  // int found = 0;
  // for (int i = 0; i < NBUCKS; i++) {
  //   if (i == index || !checklock(index, i)) {
  //     continue;
  //   }

  //   acquire(&bcache.lock[i]);    // acquire a new bucket lock
  //   b = &bcache.head[i];
  //   while (b && b->next) {
  //     if (b->next->refcnt == 0) {
  //       selected = b->next;
  //       b->next = b->next->next;
  //       found = 1;
  //       break;
  //     }
  //     b = b->next;
  //   }
  //   release(&bcache.lock[i]);

  //   if (found) {
  //     selected->dev = dev;
  //     selected->blockno = blockno;
  //     selected->valid = 0;
  //     selected->refcnt = 1;
  //     selected->next = bcache.head[index].next;  // add buffer to corresponding bucket
  //     bcache.head[index].next = selected;
  //     release(&bcache.lock[index]);
  //     acquiresleep(&selected->lock);
  //     return selected;
  //   }
  // }
  // // panic when there is no available buffer
  // panic("bgets: no buffers");

  // struct buf *b;

  // acquire(&bcache.lock);

  // // Is the block already cached?
  // for(b = bcache.head.next; b != &bcache.head; b = b->next){
  //   if(b->dev == dev && b->blockno == blockno){
  //     b->refcnt++;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  // // Not cached.
  // // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  // panic("bget: no buffers");
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
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int index = (b->blockno) % NBUCKS;
  acquire(&bcache.lock[index]);   // can be removed??
  b->refcnt--;                    // no LRU strategy here
  release(&bcache.lock[index]);
}

void
bpin(struct buf *b) {
  int index = (b->blockno) % NBUCKS;
  acquire(&bcache.lock[index]);
  b->refcnt++;
  release(&bcache.lock[index]);
}

void
bunpin(struct buf *b) {
  int index = (b->blockno) % NBUCKS;
  acquire(&bcache.lock[index]);
  b->refcnt--;
  release(&bcache.lock[index]);
}
