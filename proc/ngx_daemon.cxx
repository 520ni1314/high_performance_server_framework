//和守护进程相关
/*
公众号：程序员速成     q群：716480601
王健伟老师 《Linux C++通讯架构实战》
商业级质量的代码，完整的项目，帮你提薪至少10K
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>     //errno
#include <sys/stat.h>
#include <fcntl.h>


#include "ngx_func.h"
#include "ngx_macro.h"
#include "ngx_c_conf.h"

//描述：守护进程初始化
//执行失败：返回-1，   子进程：返回0，父进程：返回1
int ngx_daemon()
{
    /**
     * 子进程返回0
     * 父进程返回1
    */
    //(1)创建守护进程的第一步，fork()一个子进程出来
    switch (fork())  //fork()出来这个子进程才会成为咱们这里讲解的master进程；
    {
    case -1:
        //创建子进程失败
        ngx_log_error_core(NGX_LOG_EMERG,errno, "ngx_daemon()中fork()失败!");
        return -1;
    case 0:
        //子进程，走到这里直接break;
        break;
    default:
        // 父进程返回
        // 父进程以往 直接退出exit(0);现在希望回到主流程去释放一些资源【这里建议一个面试题：exit _exit Exit的区别】
        return 1;  //父进程直接返回1；
    } //end switch

    // 只有子进程继续执行
    ngx_parent = ngx_pid;     //ngx_pid是原来父进程的id，因为这里是子进程，所以子进程的ngx_parent设置为原来父进程的pid
    ngx_pid = getpid();       //当前子进程的id要重新取得
    
    // setid设置子进程为会话首进程，不在关联终端，变成守护进程
    //(2)脱离终端，终端关闭，将跟此子进程无关
    if (setsid() == -1)  
    {
        ngx_log_error_core(NGX_LOG_EMERG, errno,"ngx_daemon()中setsid()失败!");
        return -1;
    }

    /**
     * 控制新建文件或目录的权限。默认权限是0666，即任何人都可读写
     * umask(0);屏蔽字设0，允许创建的文件有所有权限
     * 目录777【执行目录中的程序】，文件666
     * umask理解：屏蔽字，权限掩码，权限掩码的作用就是屏蔽掉权限中的某些值，屏蔽掉后，权限值就为0了。比如：umask(022)，权限掩码是022，权限掩码作用在666的文件上，就是去掉022，666&(~022)=644，所以创建的文件权限是644
     * 保证了open的成功
    */
    //(3)设置为0，不要让它来限制文件权限，以免引起混乱
    umask(0); 

    //(4)打开黑洞设备，以读写方式打开
    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) 
    {
        ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中open(\"/dev/null\")失败!");        
        return -1;
    }
    if (dup2(fd, STDIN_FILENO) == -1) //先关闭STDIN_FILENO[这是规矩，已经打开的描述符，动他之前，先close]，类似于指针指向null，让/dev/null成为标准输入；
    {
        ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中dup2(STDIN)失败!");        
        return -1;
    }
    if (dup2(fd, STDOUT_FILENO) == -1) //再关闭STDIN_FILENO，类似于指针指向null，让/dev/null成为标准输出；
    {
        ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中dup2(STDOUT)失败!");
        return -1;
    }
    if (fd > STDERR_FILENO)  //fd应该是3，这个应该成立
     {
        if (close(fd) == -1)  //释放资源这样这个文件描述符就可以被复用；不然这个数字【文件描述符】会被一直占着；
        {
            ngx_log_error_core(NGX_LOG_EMERG,errno, "ngx_daemon()中close(fd)失败!");
            return -1;
        }
    }
    return 0; //子进程返回0
}

