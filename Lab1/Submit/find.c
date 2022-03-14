#include "kernel/types.h"
#include "user.h"
#include "kernel/stat.h"
#include "kernel/fs.h"

void find(char* path, char* filename);
char* fmtname(char *path);

/**
 * @brief  主函数，调用find寻找文件
 * @note   
 * @param  argc: 跟随的参数数目
 * @param  argv[]: 参数字符串数组
 * @retval 
 */
int main(int argc, char* argv[])
{
    // 需要额外附带两个参数，分别是寻找路径以及寻找的文件名
    if(argc != 3)
    {
        printf("Please input 2 argument\n");
        exit(-1);
    }
    find(argv[1], argv[2]);
    exit(0);
}

/**
 * @brief 读取路径的最后一个斜线后的内容，获取文件名
 * @note   
 * @param  *path: 文件名字符串指针
 * @retval 
 */
char* fmtname(char *path)
{
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--);
    p++;
    return p;
}

/**
 * @brief  在path路径中寻找filename文件，遇到文件夹则递归调用本函数在文件夹中寻找
 * @note   
 * @param  path: 路径
 * @param  filename: 待寻找文件名
 * @retval None
 */
void find(char* path, char* filename)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    // 判断path是否能够正常打开和解析
    if((fd = open(path, 0)) < 0)
    {
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }
    if(fstat(fd, &st) < 0)
    {
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }
    // 判断文件类型
    switch(st.type)
    {
        // 如果当前是一个文件，则判断和待寻找文件名是否相同，相同则输出
        case T_FILE:
            if(strcmp(filename, fmtname(path))==0)
            {
                printf("%s\n", path);
            }
            break;
        // 如果是文件夹则依次对文件夹下的文件/文件夹继续调用find函数进行寻找
        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
            {
                printf("ls: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            // read函数每次从fd中读取sizeof(de)个字节的内容到de当中，直到读取不到；fd每次向后移动
            while(read(fd, &de, sizeof(de)) == sizeof(de))
            {
                if(de.inum == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0)
                {
                    printf("ls: cannot stat %s\n", buf);
                    continue;
                }
                // .和..不需要进入寻找
                if(strcmp(de.name, ".")!=0 && strcmp(de.name, "..")!=0)
                    find(buf, filename);
            }
            break;
    }
    close(fd);
}