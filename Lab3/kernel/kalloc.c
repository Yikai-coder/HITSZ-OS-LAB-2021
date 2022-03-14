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

struct kmem {
  struct spinlock lock;
  struct run *freelist;
};// kmem;

struct kmem kmems[NCPU];

// void
// kinit()
// {
//   initlock(&kmem.lock, "kmem");
//   freerange(end, (void*)PHYSTOP);
// }

void kinit()
{
  push_off();
  int id = cpuid();
  char lock_name[6] = {'k', 'm', 'e', 'm', (char)(id+48), '\0'};
  // char* lock_name = snprintf("kmem_", id, "_cpu");
  initlock(&(kmems[id].lock), lock_name);
  freerange(end, (void*)PHYSTOP);
  pop_off();
  // printf("Finish init\n");
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
// void
// kfree(void *pa)
// {
//   struct run *r;

//   if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
//     panic("kfree");

//   // Fill with junk to catch dangling refs.
//   memset(pa, 1, PGSIZE);

//   r = (struct run*)pa;

//   acquire(&kmem.lock);
//   r->next = kmem.freelist;
//   kmem.freelist = r;
//   release(&kmem.lock);
// }

void kfree(void *pa)
{
    // 同原来的kfree一样处理页
    // printf("Try kfree\n");
    struct run *r;

    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
      panic("kfree");

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;
    // 将页添加到本cpu的freelist中
    push_off();
    int id = cpuid();
    acquire(&(kmems[id].lock));
    r->next = kmems[id].freelist;
    kmems[id].freelist = r;
    release(&(kmems[id].lock));
    pop_off();
    // printf("Finish kree\n");
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
// void *
// kalloc(void)
// {
//   struct run *r;

//   acquire(&kmem.lock);
//   r = kmem.freelist;
//   if(r)
//     kmem.freelist = r->next;
//   release(&kmem.lock);

//   if(r)
//     memset((char*)r, 5, PGSIZE); // fill with junk
//   return (void*)r;
// }

void * kalloc(void)
{
  // printf("Try kalloc\n");
  struct run *r;
  push_off();
  int id = cpuid();
  acquire(&(kmems[id].lock));
  r = kmems[id].freelist;

  // 首先判断自己是否有页
  if(r)
  {
    kmems[id].freelist = r->next;
    release(&(kmems[id].lock));
  }
  // 如果自己没有再看看其他CPU有没有
  else
  {
    release(&(kmems[id].lock));
    for(int i = 0; i < NCPU; i++)
    {
      if(i==id)
        continue;
      acquire(&(kmems[i].lock));
      r = kmems[i].freelist;
      if(r)
      {
        kmems[i].freelist = r->next;
        release(&kmems[i].lock);
        break;
      }
      release(&(kmems[i].lock));
    }
  }
  pop_off();
  // 将页面初始化，同原来kalloc
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
    
  // printf("Finish kalloc\n");
  return (void*)r;
}

