/*
 * @Author: liyuanze
 * @Date: 2022-08-21 23:22:34
 * @FilePath: /myTinyWebServer/connection/connection.h
 * Copyright (c) 2022 by liyuanze, All Rights Reserved. 
 */

#pragma once
#include <sys/types.h>
#include <sys/uio.h> //readv/writev
#include <arpa/inet.h> //sockaddr_in
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <exception>
#include <string.h>
#include <assert.h>
#include <stdarg.h> //va_list
#include <stdio.h> //vsnprintf
#include <sys/mman.h> //mmap
#include <sys/types.h>
#include <sys/stat.h> //stat
#include <fcntl.h> //open...
#include <sys/epoll.h>
class Connection {
public:
    enum class CHECK_STATE{
        CHECK_REQUEST_LINE=0,
        CHECK_HEADER,
        CHECK_BODY,
    };
    enum class HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST, 
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum class METHOD
    {
        GET,
        POST,
    };
    Connection();
    Connection(int sockFd);
    ~Connection();
    int readOnce();
    int writeOnce();
    int readEvent();
    int writeEvent();
    static const size_t BUFFERSIZE = 2048;
    static int epollFd;
private:
    char * _getLine();
    HTTP_CODE _parseRequestLine(char* line);
    HTTP_CODE _parseHeader(char* line);
    HTTP_CODE _parseBody();
    HTTP_CODE _checkRequest();
    HTTP_CODE _readProcess();
    void _writeProcess(HTTP_CODE ret);
    //void _reorganizeBuff();
    void _init();
    bool _addResponse(const char* format, ...);
    bool _addStatusLine(HTTP_CODE);
    bool _addHeaders(int contentLen);
    bool _addContent(const char*);
    static const char* _path;
    int _fd;
    struct iovec _iov[2];
    struct stat _fileStat;
    size_t _readBuffEnd = 0;
    size_t _writeBuffEnd = 0;
    size_t _readBufBegin = 0;
    size_t _writeBufBegin = 0;
    size_t _contentLength = 0;
    size_t _contentIndex = 0;
    size_t _fileIndex = 0;
    char _content[BUFFERSIZE];
    char _readBuff[BUFFERSIZE + 1];
    char _writeBuff[BUFFERSIZE + 1];
    char* _fileAddress;
    CHECK_STATE _checkState;
    METHOD _method;
    bool _keepAlive;
    char _fileName[20];
};