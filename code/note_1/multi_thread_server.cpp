#include "sys/socket.h"
#include "arpa/inet.h"
#include "unistd.h"
#include "cstring"
#include "stdio.h"
#include "thread"
#include "stdlib.h"
#include "vector"
#include "map"
#include "set"
#include "mutex"
#define BUFFER_MAX 128


int THREAD_MAX;
std::mutex mtx;
/* 线程池 */
std::vector<std::thread> work_threads;
/* 空闲线程 */
std::set<int> free_index_set;

/* 判断是否出错 */
void iferror(bool check,const char* error_msg){
    if(check){
        perror(error_msg);
        exit(0);
    }
}
/* 工作线程 */
void work(int clntfd,int index){
    perror("a new connection accept");
    while(true){
        char buffer[BUFFER_MAX];
        memset(buffer,0,sizeof(buffer));
        int ret = read(clntfd,buffer,BUFFER_MAX);
        if(ret <=0){
            perror("out of connction");
            std::lock_guard<std::mutex> lock(mtx);
            free_index_set.insert(index);
            break;
        }
        ret = write(clntfd,buffer,BUFFER_MAX);
        iferror(ret<=0,"write fail");
    }
    close(clntfd);
}


/* argv[]存放服务器ip port*/
int main(int args,char* argv[]){
    int servfd = socket(AF_INET,SOCK_STREAM,0);

    if(args <= 3){
        perror("lack args");
        return 0;
    }
    int opt=1;
    if (setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(servfd);
        return 1;
    }
    THREAD_MAX = atoi(argv[3]);
    work_threads.resize(THREAD_MAX);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    int ret=bind(servfd,(sockaddr*)&serv_addr,sizeof(serv_addr));
    iferror(ret < 0,"bind fail");
    ret=listen(servfd,SOMAXCONN);
    iferror(ret < 0,"listen fail");
    /* 上面和基本的C/S模型一致 */

    /* 初始化空闲集合 */
    for(int i=0;i<THREAD_MAX;i++){
        free_index_set.insert(i);
    }
    /* 循环进行accept,每当处理完一个accept,就将该socket加入到工作线程 */
    while(true){
        struct sockaddr_in clnt_addr;
        memset(&clnt_addr,0,sizeof(clnt_addr));
        socklen_t clnt_addr_len = sizeof(clnt_addr);
        int clntfd = accept(servfd,(sockaddr*)&clnt_addr,&clnt_addr_len);
        iferror(ret < 0,"accpet fail");
        if(free_index_set.size()  == 0){
            perror("too many work thread");
            close(clntfd);
            continue;
        }
        /* 从空闲集合中取一个空闲下标 */
        int free_index = *free_index_set.begin();
        free_index_set.erase(free_index);
        work_threads[free_index] = std::thread(work,clntfd,free_index);
    }
    /* 主线程等待尚未完成的工作线程 */
    for(int i=0;i<work_threads.size();i++){
        work_threads[i].join();
    }
    close(servfd);
}