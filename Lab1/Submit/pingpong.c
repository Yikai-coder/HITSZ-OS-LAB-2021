#include "kernel/types.h"
#include "user.h"

/**
 * @brief  主函数，开启两个管道，父进程向管道1写然后在管道2读，子进程在管道1读然后在管道2写
 * @note   
 * @param  argc: 
 * @param  argv[]: 
 * @retval 
 */
int main(int argc, char* argv[])
{
    int fd1[2], fd2[2];
    pipe(fd1);
    pipe(fd2);
    int pid;
    // 父进程
    if((pid = fork())!=0)
    {
        char send [] = "ping";
        char receive[4];
        int n;
        close(fd1[0]);
        close(fd2[1]);
        if((n = write(fd1[1], send, 4)==-1))
        {
            printf("Fail to write to pipe\n");
            exit(-1);
        }
        close(fd1[1]);
        if((n = read(fd2[0], receive, 4))==-1)
        {
            printf("Fail to read from the pipe\n");
            exit(-1);
        }

        close(fd2[0]);

        printf("%d: received %s\n", getpid(), receive);
        wait(0);
    }
    // 子进程
    else
    {
        char send [] = "pong";
        char receive[4];
        int n;
        close(fd1[1]);
        close(fd2[0]);
        if((n = read(fd1[0], receive, 4))==-1)
        {
            printf("Fail to read from the pipe\n");
            exit(-1);
        }
        close(fd1[0]);
        printf("%d: received %s\n", getpid(), receive);
        if((n = write(fd2[1], send, 4)==-1))
        {
            printf("Fail to write to pipe\n");
            exit(-1);
        }
        close(fd2[1]);
        exit(0);
    }
    exit(0);
}
