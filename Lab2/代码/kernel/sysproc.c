#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
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

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
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
/**
 * @brief  系统调用trace，为进程设定系统调用的输出，格式为PID：sys_&{name}(arg0) -> retval
 * @note   
 * @retval Fail:-1 Success:0
 */
uint64 sys_trace(void)
{
    int mask;
    // 异常，没有接收到mask参数
    if(argint(0, &mask) < 0)
      return -1;
    // 给进程设置mask
    struct proc *p = myproc();
    p->mask = mask;

    return 0;
}

/**
 * @brief  系统调用打印进程相关信息，包括：剩余内存空间，剩余可使用的进程数，剩余文件描述符
 * @note   
 * @retval Fail:-1 Success:0
 */
uint64 sys_info(void)
{
  uint64 addr;
  if(argaddr(0, &addr)<0)
    return -1;
  struct proc *p = myproc();
  struct sysinfo info;

  info.freemem = cal_free_mem();
  info.nproc = check_unused_proc();
  info.freefd = check_remaining_fd();

  if(copyout(p->pagetable, addr, (char *)&info, 24) < 0)
    return -1;

  return 0;
}
