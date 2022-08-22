/*
 * @Author: liyuanze
 * @Date: 2022-08-21 23:34:08
 * @FilePath: /myTinyWebServer/webserver.cpp
 * Copyright (c) 2022 by liyuanze, All Rights Reserved. 
 */

#include "webserver.h"

using namespace std;
WebServer::~WebServer(){}
WebServer::WebServer(int port, int timeoutMS, int threadNum):_port(port), _timeoutMS(timeoutMS), _threadPool(threadNum) {
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(_port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    _listenFd = socket(PF_INET, SOCK_STREAM, 0);
    assert(_listenFd > 0);
    int ret = bind(_listenFd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(_listenFd, 5);
    assert(ret != -1);
    _setFdNonblock(_listenFd);

    _epollFd = epoll_create(5);
    Connection::epollFd = _epollFd;
    struct epoll_event event;
    event.data.fd = _listenFd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(_epollFd, EPOLL_CTL_ADD, _listenFd, &event);

}

void WebServer::loop() {
    while (true)
    {
        int eventNum = epoll_wait(_epollFd, _epollEvents, MAX_EVENT_NUM, -1);
        puts("loop");
        printf("%d\n", eventNum);
        for (int i = 0; i < eventNum; i++)
        {
            int fd = _epollEvents[i].data.fd;
            int events = _epollEvents[i].events;
            if(fd == _listenFd) {
                _threadPool.submit(std::bind(&WebServer::_dealListen, this));
            }
            else if(events & EPOLLHUP) {//两边连接全部关闭（本程序一般不会发生）
                close(fd);
            }
            else if(events & EPOLLIN) {
                _threadPool.submit(std::bind(&WebServer::_readEvent, this, fd));
            }
            else if(events & EPOLLOUT) {
                _threadPool.submit(std::bind(&WebServer::_writeEvent, this, fd));
            }
        }
        
    }
    
}

void WebServer:: _dealListen() {
    do
    {
        struct sockaddr_in address;
        socklen_t socklen = sizeof(address);
        int fd = accept(_listenFd, (sockaddr*)&address, &socklen);
        if(fd==-1 && (errno==EAGAIN||errno==EWOULDBLOCK)) { //这里是读完了
            return;
        }
        else if(fd==-1) {
            std::exception();
        }
        _conn.emplace(std::piecewise_construct, std::forward_as_tuple(fd), std::forward_as_tuple(fd));
        struct epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLET | EPOLLONESHOT;
        int ret = epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event);
        _setFdNonblock(fd);
        assert(ret != -1);
    } while (true);
}

void WebServer:: _writeEvent(int fd) {
     Connection* conn = &_conn[fd];
     if(conn->writeEvent() == -1) {
        _conn.erase(fd);
        //int ret = epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL);
        //assert(ret != -1);
        close(fd);
     }
}
void WebServer::_readEvent(int fd) {
    puts("_readEvent");
    Connection* conn = &_conn[fd];
    if(conn->readEvent() == -1) {
        _conn.erase(fd);
        //int ret = epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL);
        //assert(ret != -1);
        close(fd);
    }
}


void WebServer::_setFdNonblock(int fd) {
    int ret = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
    assert(ret != -1);
}