/*
 * @Author: liyuanze
 * @Date: 2022-08-21 23:36:51
 * @FilePath: /myTinyWebServer/webserver.h
 * Copyright (c) 2022 by liyuanze, All Rights Reserved. 
 */

#pragma once
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "./threadPool/threadPool.h"
#include "./connection/connection.h"

class WebServer {
public:
    WebServer(
        int port, int timeoutMS, int threadNum
    );
    ~WebServer();
    void loop();
private:
    int _port;
    int _timeoutMS;
    int _epollFd;
    int _listenFd;
    threadPool _threadPool;
    std::unordered_map<int, Connection> _conn;
    static const int MAX_EVENT_NUM = 10000;
    struct epoll_event _epollEvents[MAX_EVENT_NUM];
    void _setFdNonblock(int fd);
    void _dealListen();
    void _readEvent(int fd);
    void _writeEvent(int fd);
};