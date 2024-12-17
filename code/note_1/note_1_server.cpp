#include "sys/socket.h"
#include "arpa/inet.h"
#include "unistd.h"
#include "cstring"
#include "stdio.h"
#include "stdlib.h"
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
    //创建socket
    //(地址族,套接字协议(TCP/UDP),(此处是选择协议，0是根据前两个参数自动选择))
    int servfd = socket(AF_INET,SOCK_STREAM,0);

    //创建sockaddr,初始化地址并进行地址设置
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    if(args <= 2){
        perror("lack args");
        return 0;
    }
    //设置地址族
    serv_addr.sin_family = AF_INET;
    //设置ip
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    //设置port
    serv_addr.sin_port = htons(atoi(argv[2]));
    //绑定socket与sockaddr
    int ret=bind(servfd,(sockaddr*)&serv_addr,sizeof(serv_addr));
    iferror(ret < 0,"bind fail");
    //监听服务器socket
    //(监听的socket,监听队列长度)
    ret=listen(servfd,SOMAXCONN);
    iferror(ret < 0,"listen fail");
    //创建客户端 sockaddr
    struct sockaddr_in clnt_addr;
    memset(&clnt_addr,0,sizeof(clnt_addr));
    socklen_t clnt_addr_len = sizeof(clnt_addr);
    //接受连接,获得客户端的socket
    int clntfd = accept(servfd,(sockaddr*)&clnt_addr,&clnt_addr_len);
    iferror(ret < 0,"accpet fail");
    //进行通信
    while(true){
        char buffer[BUFFER_MAX];
        memset(buffer,0,sizeof(buffer));
        ret = read(clntfd,buffer,BUFFER_MAX);
        iferror(ret <=0 ,"read fail");
        ret = write(clntfd,buffer,BUFFER_MAX);
        iferror(ret<=0,"write fail");
    }
    close(servfd);
    close(clntfd);
}