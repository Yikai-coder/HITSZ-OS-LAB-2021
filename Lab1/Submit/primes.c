#include "kernel/types.h"
#include "user.h"

void get_prime(int fd[2]);

/**
 * @brief  主函数，创建子进程和管道，父进程向管道中写入所有的数，最后添上1个0，用以表示读取到末尾
 * @note   
 * @param  argc: 
 * @param  argv[]: 
 * @retval 
 */
int main(int argc, char* argv[])
{
    int fd[2];
    pipe(fd);
    // 子进程
    if(fork()==0)
    {
        get_prime(fd);
    }
    else
    {
        close(fd[0]);
        for(int i = 2; i < 36; i++)
        {
            write(fd[1], &i, 1);
        }
        // 序列最后一个数为0
        int i = 0;
        write(fd[1], &i, 1);
        wait(0);
        exit(0);
    }
    exit(0);
}

/**
 * @brief  子进程函数，每个子进程从管道中读取数据，然后选取第一个为prime，接着创建一个管道和子进程，向该管道写入不能被prime整除的数，
 * * 然后等待子进程；子进程进一步调用prime。直到读取到prime为-1，此时所有数都被筛选出了，开始退出
 * @note   
 * @param  fd[2]: 管道的文件描述符
 * @retval None
 */
void get_prime(int fd[2])
{
    close(fd[1]);
    int p;
    int fd1[2];
    pipe(fd1);
    read(fd[0], &p, 1);
    // 读取到0为第一个数，说明已经没有数了
    if(p==0)
        exit(0);
    printf("prime %d\n", p);
    if(fork()==0)
    {
        get_prime(fd1);
    }
    else
    {
        close(fd1[0]);
        while(1)
        {
            int n;
            read(fd[0], &n, 1);
            // 最后读取到0重新往后写入管道
            if(n==0)
            {            
                write(fd1[1], &n, 1);
                break;
            }
            if(n%p != 0)
            {
                write(fd1[1], &n, 1);
            }
        }
        wait(0);
        exit(0);
    }    
}