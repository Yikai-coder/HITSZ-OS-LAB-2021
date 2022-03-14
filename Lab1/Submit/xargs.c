#include "kernel/types.h"
#include "kernel/param.h"
#include "user.h"
#include "stddef.h"

/**
 * @brief  主函数，读取stdin，然后创建一个子进程，子进程执行原来的命令加上新添加的参数
 * 父进程等待子进程执行完毕，进入下一次循环
 * @note char* str[]没有分配空间，所以直接把其它字符串的指针赋值给str[i]，字符串没有复制；strcmp只能用于比较字符串，单个字符的比较会导致出错
 * @param  argc: 
 * @param  argv[]: 
 * @retval 
 */
int main(int argc, char* argv[])
{
    char buf[MAXARG];
    char *exec_argv[MAXARG];

    // 注意argv的修改会导致不可预测的结果，所以这里把不会被修改且有用的的argv[1-(argc-1)]的指针直接给到exec_argv
    // 同时由于exec_argv没有分配初始空间，不能用strcpy来赋值
    // exec_argv的最后两项分别是读入的输入和0，也通过别处赋值
    for(int i = 1; i < argc; i++)
    {
        exec_argv[i-1] = argv[i];
    }
    while(1)
    {
        int i = 0;
        for(i = 0; i < MAXARG; i++)
        {
            int len = read(0, &buf[i], 1);
            // if(strcmp(&buf[i], "\n")==0 || len <= 0)   // 不要使用strcmp进行比较，会出现问题！！！
            if(buf[i] == '\n' || len <= 0)
                break;
        }
        if(i==0)
            break;
        // buf最后一位设置为0
        buf[i]=0;
        // exec_argv初始没有分配空间，所以还是直接把指针ca给到exec_argv
        exec_argv[argc-1] = buf;
        exec_argv[argc] = 0;
        // 子进程
        if(fork()==0)
        {
            exec(exec_argv[0], exec_argv);
            exit(0);
        }
        // 父进程
        else
        {
            wait(0);
        }
    }
    exit(0);
}