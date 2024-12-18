#include "sys/socket.h"
#include "arpa/inet.h"
#include "unistd.h"
#include "cstring"
#include "stdio.h"
#include "stdlib.h"
#include "set"
#include "sys/select.h"
#define BUFFER_MAX 128

/* 判断是否出错 */
void iferror(bool check,const char* error_msg){
    if(check){
        perror(error_msg);
        exit(0);
    }
}

/* argv[]存放服务器ip port*/
int main(int args,char* argv[]){
    int servfd = socket(AF_INET,SOCK_STREAM,0);

    if(args <= 2){
        perror("lack args");
        return 0;
    }
    int opt=1;
    if (setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(servfd);
        return 1;
    }
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

    /* 创建读就绪文件描述符集合 */
    fd_set readfds;
    /* 创建文件描述符集合(便于找最大值以及初始化) */
    std::set<int> fds;
    fds.insert(servfd);
    int fd_max=servfd;    
    while(true){
        /* 初始化readfds */
        FD_ZERO(&readfds);
        /* 设置需要监听的文件描述符 */
        for(auto &fd:fds){
            FD_SET(fd,&readfds);
        }
        /* 获取就绪的文件描述符 */
        int ret = select(fd_max+1,&readfds,NULL,NULL,NULL);
        iferror(ret<=0,"select fail");
        for(auto &fd:fds){
            /* 对服务器socket的读就绪，即有连接来临 */
            if(FD_ISSET(fd,&readfds) && fd == servfd){
                struct sockaddr_in clnt_addr;
                memset(&clnt_addr,0,sizeof(clnt_addr));
                socklen_t clnt_addr_len = sizeof(clnt_addr);
                int clntfd = accept(servfd,(sockaddr*)&clnt_addr,&clnt_addr_len);
                iferror(ret < 0,"accpet fail");
                FD_SET(clntfd,&readfds);
                fds.insert(clntfd);
                fd_max = *(--fds.end());
            }
            /* 对于客户端socket的读就绪，即有信息来临 */
            else if(FD_ISSET(fd,&readfds)){
                char buffer[BUFFER_MAX];
                memset(buffer,0,sizeof(buffer));
                ret = read(fd,buffer,BUFFER_MAX);
                if(ret <= 0){
                    perror("out of connection");
                    FD_CLR(fd,&readfds);
                    fds.erase(fd);
                    close(fd);
                    continue;
                }
                ret = write(fd,buffer,BUFFER_MAX);
                iferror(ret<=0,"write fail");
            }
        }        

    }
    close(servfd);
}