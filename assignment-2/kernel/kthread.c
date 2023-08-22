#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern struct proc proc[NPROC];


// A fork child's very first scheduling by scheduler()4
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&mykthread()->lock);
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

struct kthread *mykthread()// check my tread with Alina
{
  push_off();
  struct cpu *c = mycpu();
  struct kthread *t;
  if(c != 0 && c->kt!= 0)
  {
    t = c ->kt;
    pop_off();
    return t;
  }
  pop_off();
  return 0;
  }


void kthreadinit(struct proc *p)
{
  initlock(&p->tid_lock, "tid_lock");

  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    initlock(&kt->lock, "tlock");
    kt->state = UNUSED;
    kt->parent = p; 
    // WARNING: Don't change this line!
    // get the pointer to the kernel stack of the kthread
    kt->kstack = KSTACK((int)((p - proc) * NKT + (kt - p->kthread)));
  }
}


struct trapframe *get_kthread_trapframe(struct proc *p, struct kthread *kt)
{
  return p->base_trapframes + ((int)(kt - p->kthread));
}

int
alloctid(struct proc *p)
{
  int tid;
  acquire(&p ->tid_lock);
  tid =  p->counter_tid;
  p->counter_tid = p->counter_tid + 1;
  release(&p ->tid_lock);
  return tid;
}


// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
void
freekthread(struct kthread *kt)
{
  if(kt->trapframe)
    kfree((void*)kt->trapframe);
  kt ->trapframe = 0;
  kt->tid = 0;
  kt->parent = 0;
  kt->chan = 0;
  kt->killed = 0;
  kt->xstate = 0;
  kt->state = UNUSED;
}

struct kthread*
allockthread(struct proc *p)
{
  struct kthread *kt;

  for(kt = p->kthread; kt < &p->kthread[NKT]; kt++) {
    acquire(&kt -> lock);
    if(kt->state == UNUSED) {
      goto found;
    } else {
      release(&kt -> lock); 
    }
  }
  return 0;

found:
  kt->tid = alloctid(p);
  kt->state = USED;

  // Allocate a trapframe page.
  //struct trapframe *tp = get_kthread_trapframe(p, kt);
  if((kt ->trapframe = (struct trapframe *)kalloc()) == 0){
    freekthread(kt);
    release(&kt -> lock);  
    return 0;
  }
  // Set up new context to start executing at forkret,
  // which returns to user space.
  kt->trapframe = get_kthread_trapframe(p, kt);
  memset(&kt->context, 0, sizeof(kt->context));
  kt->context.ra = (uint64)forkret;
  kt->context.sp = kt->kstack + PGSIZE;

  return kt;
}