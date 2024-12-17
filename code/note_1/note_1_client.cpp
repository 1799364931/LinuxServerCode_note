#include "sys/socket.h"
#include "arpa/inet.h"
#include "unistd.h"
#include "cstring"
#include "stdio.h"
#include "stdlib.h"
#define BUFFER_MAX 128
void iferror(bool check,const char* error_msg){
    if(check){
        perror(error_msg);
        exit(0);
    }
}

int main(int args,char* argv[]){
    //创建服务器socket
    int servfd = socket(AF_INET,SOCK_STREAM,0);
    //创建并设置服务器sockaddr
    struct sockaddr_in sevr_addr;
    memset(&sevr_addr,0,sizeof(sevr_addr));
    if(args <= 2){
        perror("lack args");
        return 0;
    }

    sevr_addr.sin_family = AF_INET;
    sevr_addr.sin_addr.s_addr = inet_addr(argv[1]);
    sevr_addr.sin_port = htons(atoi(argv[2]));
    //连接服务器,获取对方的通信sockets
    int ret=connect(servfd,(sockaddr*)&sevr_addr,sizeof(sevr_addr));
    iferror(ret < 0,"connect fail");
    //进行通信收发
    while(true){
        char buffer[BUFFER_MAX];
        memset(buffer,0,sizeof(buffer));
        printf("> ");
        scanf("%s",buffer);
        write(servfd,buffer,BUFFER_MAX);
        int ret = read(servfd,buffer,BUFFER_MAX);
        iferror(ret <=0 , "out of connection");
        printf("receive message: %s \n",buffer);
    }

    close(servfd);

}