# Linux 网络编程 --- 1
## 写在前面
本(系列)文章主要记录和分享笔者学习Linux网络编程的过程，对笔者认为较难理解的概念或代码进行二次阐释与理解。\
主要参考学习书籍 《Linux高性能服务器编程》--游双著 

## 在开始之前
由于笔者本学期在校修读了《计算机网络》课程，但无奈课程仅仅讲述计算机网络的理论部分，对具体的网络编程方面几乎没有涉及，因此自行学习Linux网络编程。\
故下文所涉及到基础的网络概念不作解释，默认读者有基本的计算机网络理论知识概念。

本(系列)文章希望通过一个简单的C/S模型，不断为其增加功能 or 提高性能来引入新的网络编程知识与技术，相较于先抛出各种api或概念，在一个自己熟悉的框架上进行不断的建筑和修改的学习方法，这种方法笔者认为更适合知识的学习和分享。

## 一切皆文件
在Linux中，**一切皆文件**。Linux网络编程也是建立在这个概念之上的，对于不同实体的网络通信，网络将实体抽象为一个``socket``;``socket``在Linux中以文件描述符的形式出现，即网络中的通信实体被Linux抽象为``文件``概念。

在网络通信时，将通信的双方抽象为``文件``，可以利用``文件``的读写概念进行通信的收发。

## 简单的C/S模型
在C/S模型中，服务器需要守候在其特定端口监听客户机的连接，客户机需要事先知道服务器的地址以请求连接。\
下面的代码实现了一个简单的C/S模型，server先守候在端口，等待client进行连接，连接成功后client可通过终端输入发送信息，server回送收到的信息。
### Server
```cpp
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
```

### Client
```cpp
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
```

## 多连接问题
基于上面简单的C/S模型，我们可以实现一个简单的单对单的通信。\
为什么是单对单？由于accept是阻塞的，所以服务器只能等待accept调用返回后才能进行连接处理；在进行连接处理的过程中的read和write调用依旧是阻塞的，所以当有多个连接进行访问时，服务器无法对其进行一一的处理。\
所以如何解决服务器只能处理一个连接的缺点呢？

### 多线程
第一个比较容易想到的方法就是多线程，既然一个线程只能处理一个连接，那只需要在接受连接后创建一个处理收发的线程，然后主线程继续进行监听以接受连接，这样就可以实现多线程处理多连接了。

下面给出示例代码(PS:示例代码的工作线程是接受后创建的(这样比较慢)，事实上可以事先创建好工作线程池，然后利用一个队列进行socket的处理，但这属于后面的内容，此处不详细说明)
```cpp
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
```

### I/O多路复用
#### 多线程之痛
多线程确实能解决多连接的问题，但是对于一些核数较少的CPU而言，多线程会带来频繁的上下文切换开销。\
所以，**I/O多路复用**带来了一种单线程实现多连接处理的方法。

#### I/O就绪和I/O处理
讲述I/O多路复用之前，需简要讲述**就绪**和**处理**的区别。\
对一个I/O操作而言，其时间片可以分为**等待IO就绪**，**处理IO(数据拷贝)**。\
下面将从两个简单的例子来理解它们的意义
- ``accept``\
当没有访问来临时，其会阻塞调用线程，等待``listen``的队列中出现访问到来。(**等待IO就绪**)\
当访问来临时，accept就会尝试发起连接。(**IO处理**)
- ``read`` \
当缓冲区没有任何数据可读时，其会阻塞调用线程，等待缓冲区可读。(**等待IO就绪**)\
当缓冲区存在可读数据时，其读取缓冲区数据。(**IO处理**)

#### 阻塞与非阻塞
- **阻塞** 是指线程在等待IO时被挂起，只有IO处理完毕后线程才能继续执行。

- **非阻塞** 是指线程在调用IO时，无需等待IO处理结果，直接返回执行。

因此阻塞与非阻塞的区别在于线程是否等待IO就绪。
#### 同步与异步
- **同步** 是指线程的执行与IO的处理同步，即线程来进行IO处理(但线程不一定要一直等待IO就绪)

- **异步** 是指线程的执行与IO的处理异步，即线程不关心IO的处理过程，只关心IO的处理结果。

因此同步与异步的区别在于线程是否关心IO就绪的结果，是否需要通过IO就绪的结果来执行IO处理。

#### 同步阻塞/同步非阻塞/异步阻塞/异步非阻塞
- **同步阻塞** 线程既要挂起等待IO就绪，也要执行IO处理。

- **同步非阻塞** 线程不等待IO就绪，线程只希望在IO就绪时进行IO处理。但此时由于线程希望执行IO处理，所以线程必须得轮询IO就绪结果，在IO就绪时进行处理。

- **异步阻塞** 线程在执行一个异步操作后需要阻塞等待异步操作结果的通知才能继续执行。(似乎不存在IO的异步阻塞)

- **异步非阻塞** 线程既不等待IO就绪，也不关心IO处理。线程只希望获得IO的处理结果。

#### I/O多路复用
铺垫了那么久终于到正题了，I/O多路复用即对单个文件描述符的阻塞等待变成了同时阻塞等待。即一个线程可以同时监视多个``socket``的就绪状态，并且在对应监听的就绪状态发生时予以返回，通知主线程进行处理。

也就是说，I/O多路复用**本身是阻塞的**。当其监听的``socket``没有满足它所监听的就绪状态时，其会阻塞等待事件的发生。(但是I/O多路复用一般会存在计时器，当计时器设置为0时，可以认为是非阻塞的，因为无需等待直接返回)

并且，当多个事件发生时候，单线程的I/O多路复用是无法做到同时处理所有事件，其只能按顺序处理事件。所以在实际上，单线程依然是在串行工作。

用大白话来讲，就是线程将所有的文件描述符扔给了操作系统，让操作系统去监听线程所期待的事件发生，当事件发生时(IO就绪)，操作系统通知线程去处理事件。

#### I/O多路复用系统调用
I/O多路复用在Linux下有``select``,``poll``,``epoll``三种系统调用，下面将简要讲述这三种系统调用的使用方法以及区别。

##### select
