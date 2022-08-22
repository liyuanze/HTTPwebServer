/*
 * @Author: liyuanze
 * @Date: 2022-08-21 23:59:04
 * @FilePath: /myTinyWebServer/main.cpp
 * Copyright (c) 2022 by liyuanze, All Rights Reserved. 
 */

#include <unistd.h>
#include "webserver.h"
int Connection::epollFd = -1;
int main() {
    //daemon(1, 0); 
    WebServer server(1316, 60000, 6); /* 端口 timeoutMs 线程池数量 */
    server.loop();
} 
  