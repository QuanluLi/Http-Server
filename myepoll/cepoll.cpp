#include "cepoll.h"

Epoll::Epoll(int size, int max_events) : max_events(max_events) {
    evlist = new struct epoll_event;
    EpollCreate(size);
}

Epoll::~Epoll() {
    delete[] evlist;
}

int Epoll::EpollCreate(int size) {
    epoll_fd = epoll_create(size);     //创建新的epoll句柄，size是监听的数目。epoll_create(int size)会占用一个fd，使用结束后要close这个fd
    if(epoll_fd == -1) {
        errExit("epoll create error");
        return -1;
    }
    return 0;
}

int Epoll::AddETIN(int fd) const {  //fd是要添加进epoll_fd进行监听的描述符
    struct epoll_event ev;   //epoll_event结构体记录要监听的信息
    ev.events = ETIN;    //监听fd的可读事件
    ev.data.fd = fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {        //事件注册函数int epoll_ctl(int epfd, int op, int fd, struct epoll_event*event);
        errExit("add read event failed");
        return -1;
    }
    return 0;
}

int Epoll::AddETOUT(int fd) const {
    struct epoll_event ev;
    ev.events = ETOUT;
    ev.data.fd = fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        errExit("add write event failed");
        return -1;
    }
    return 0;
}

int Epoll::ModEvent(int fd, ETOP op) const {
    struct epoll_event ev;
    ev.events = op;
    ev.data.fd = fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {      //修改已经注册的fd的监听事件
        errExit("modify epoll event failed");
        return -1;
    }
    return 0;
}

int Epoll::WaitEvent(int timeout) {
    int ready;
    // readyFd.clear();
    ready = epoll_wait(epoll_fd, evlist, max_events, timeout);   //等待监听的事件发生，超时返回0，错误返回-1， 就绪态事件的个数nfds，evlist记录就绪态事件的集合
    if(ready == -1) {
        errExit("epoll_wait error");
        return -1;
    }
    if(ready == 0) {
        errExit("beyond time");
        return 0;
    }
    // for(int j = 0; j < ready; ++j) {
    //     //if(evlist[j].events & EPOLLIN)
    //         readyFd.push_back(evlist[j].data.fd);
    // }
    return ready;
}

int Epoll::GetEventFd(int n) {  //返回第n个就绪态事件的fd
    return evlist[n].data.fd;
}

bool Epoll::isReadAvailable(int n) {
    return evlist[n].events & EPOLLIN ? true : false;
}

bool Epoll::isWriteAvailable(int n) {
    return evlist[n].events & EPOLLOUT ? true : false;
}