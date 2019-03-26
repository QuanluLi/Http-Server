/*  twebserv.cpp - a threaded minimal web server (version 0.2)
*  usage: tws portnumbe
*feature: supports the GET ommand only
*         runs in the current directory
*         creates a thread to handle each request
*         suppports a special status URL to reprot internal state
*  build: cc twebser.cpp socklib.c -lpthread -o twebserv
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <unordered_map>
#include <signal.h>

#include "handle.h"
#include "socklib.h"
#include "common.h"
#include "./threadpool/cthreadpool.h"
#include "./threadpool/mutex.h"
#include "./myepoll/cepoll.h"

/* server facts here */
time_t server_started;//请求时间
int server_bytes_sent;//发送字节总数
int server_requests; //请求总次数

std::unordered_map<int, user_data> um;
int nfds;
Mutex mapmux; //给map加锁
Epoll myepoll(30, 10);

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    if(argc < 3) {
        fprintf(stderr, "请指定端口号和工作路径！\n\fasheng
        示例:\n./twebserv -p 8888 [-d /home/] []参数可省略\n");
        exit(1);
    }

    char dir[BUFSIZ] = "./htdocs";
    int port = -1;
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-d") ==0)
            strcpy(dir, argv[++i]);
    }
    if(port == -1) {
        fprintf(stderr, "%d\n", port);
        fprintf(stderr, "%s\n", dir);
        fprintf(stderr, "输入参数不正确，请重新输入！\n");
        fprintf(stderr, "请指定端口号和工作路径！\n\
        示例: ./twebserv -p 8888 -d /home/\n");
        exit(1);
    }

    chdir(dir);
    int sock, fd;
    // pthread_t worker;
    // pthread_attr_t attr;

    sock = make_server_socket(port);
    if(sock == -1) {
        perror("making socket");
        exit(2);
    }

    CThreadPool threadpool(30);

    // setup(&attr);//置独立线程，即线程结束后无需调用pthread_join阻塞等待线程结束，忽略SIGPIPE信号

    // /* main loop here: take call, handle call in new thread */
    // while(true) {
    //     fd = accept(sock, NULL, NULL);
    //     server_requests++;
    //     fdptr = (int *)malloc(sizeof(int));
    //     *fdptr = fd;
    //     // pthread_create(&worker, &attr, handle_call, fdptr);
    //     CMyTask *taskObj = new CMyTask(&handle_call, (void*)fdptr);
    //     threadpool.AddTask(taskObj);
    // }

    // int epfd = epoll_create(30);
    // struct epoll_event ev, events[10];
    // ev.data.fd = sock;
    // ev.events = EPOLLIN | EPOLLET;
    // epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);
    myepoll.AddETIN(sock);

    while(true) {
        // nfds = epoll_wait(epfd, events, 10, -1);
        nfds = myepoll.WaitEvent(-1);  //用timeout=-1表示一直等下去，没有超时限制，适用于网络主循环为单独线程的情况
        for(int i = 0; i < nfds; i++) {
            if(myepoll.GetEventFd(i) == sock) {
                fd = accept(sock, NULL, NULL);    //accept不会返回传入的fd，而是会分配一个新的fd作为链接的文件描述符，传入的fd依然保留等待链接的到来
                if(fd < 0)
                    continue;
                myepoll.AddETIN(fd);   //将新连接添加进epoll，等待事件发生
            }
            else if(myepoll.isReadAvailable(i)) {
                int *fdptr = (int *)malloc(sizeof(int));
                *fdptr = fd;
                CMyTask *taskObj = new CMyTask(&handle_call, (void *)fdptr);
                threadpool.AddTask(taskObj);
            }
            else if(myepoll.isWriteAvailable(i)) {
                int *fdptr = (int *)malloc(sizeof(int));
                *fdptr = fd;
                CMyTask *taskObj = new CMyTask(&process_rp, (void *)fdptr);
                threadpool.AddTask(taskObj);
            }
            else {
                printf("error\n");
                exit(1);
            }
        }
    }

    return 0;
}