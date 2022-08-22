/*
 * @Author: liyuanze
 * @Date: 2022-08-21 23:31:29
 * @FilePath: /myTinyWebServer/connection/connection.cpp
 * Copyright (c) 2022 by liyuanze, All Rights Reserved. 
 */


#include "connection.h"

//定义http响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

const char* Connection::_path = "../resources/";

Connection::Connection(){}
Connection::~Connection(){}
Connection::Connection(int sockFd) {
    _fd = sockFd;
    _writeBufBegin = 0;
    _writeBuffEnd = 0;
    _readBuffEnd = 0;
    _writeBuffEnd = 0;
    _contentLength = 0;
    _contentIndex = 0;
    _fileIndex = 0;
    _keepAlive = false;
    _checkState = CHECK_STATE::CHECK_REQUEST_LINE;
}

void Connection::_init() {
    _writeBufBegin = 0;
    _writeBuffEnd = 0;
    _readBuffEnd = 0;
    _writeBuffEnd = 0;
    _contentLength = 0;
    _contentIndex = 0;
    _fileIndex = 0;
    _keepAlive = false;
    _checkState = CHECK_STATE::CHECK_REQUEST_LINE;
}

/**
 * @description: 读socket
 * @return 1:成功读到，-1：异常，关闭连接
 */
int Connection::readOnce() { 
    assert(_readBuffEnd != Connection::BUFFERSIZE);
    do
    {
        if (_readBuffEnd == Connection::BUFFERSIZE)
        {
            return 1;
        }
        int ret = read(_fd, _readBuff + _readBuffEnd, Connection::BUFFERSIZE - _readBuffEnd);
        if(ret==0) {
            return -1;
        }
        else if(ret==-1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                return 1;
            }
            return -1;
        }
        _readBuffEnd += ret;
        _readBuff[_readBuffEnd] = '\0';
    } while (true);  
}
/**
 * @description: 
 * @return 1：写完了，-1：没写完，注册回写事件，-2：出错
 */
int Connection::writeOnce() {
    size_t all = _iov[0].iov_len + _iov[1].iov_len;
    printf("writeOnce:%d %d\n", _iov[0].iov_len, _iov[1].iov_len);
    do
    {
        int ret = writev(_fd, _iov, 2);
        printf("writeOnce:ret->%d\n", ret);
        if(ret < 0) {
            if(ret == -1 && errno & (EAGAIN | EWOULDBLOCK)) {
                return -1;
            }
            return -2;
        }
        if(ret < _iov[0].iov_len) {
            _iov[0].iov_len -= ret;
            _writeBufBegin += ret;
            _iov[0].iov_base = _writeBuff + _writeBufBegin;
        }
        else  {
            all -= _iov[0].iov_len;
            _iov[0].iov_len = 0;
            _iov[1].iov_len -= all;
            _fileIndex += all;
            _iov[1].iov_base = _fileAddress + _fileIndex;
        }
        if(0 == _iov[0].iov_len + _iov[1].iov_len) {
            return  1;
        }
        printf("%d\n", _iov[0].iov_len + _iov[1].iov_len);
    } while (true);
    
}
/**
 * @description: 返回readBuff完整行的第一个字符的指针，如果没有完整行返回空指针
 * @return pointer to char or nullptr
 */
char* Connection::_getLine() {
    printf("_getLine:%d %d\n", _readBufBegin, _readBuffEnd);
    char * line = _readBuff + _readBufBegin;
    if(_readBufBegin == _readBuffEnd) {
        return nullptr;
    }
    char* next = strstr(_readBuff + _readBufBegin, "\r\n");
    if(!next) {
        return nullptr;
    }
    *(next) = *(next + 1) = '\0';
    _readBufBegin = next - _readBuff + 2;
    return line;
}
/**
 * @description: 读readbuff, 状态机核心推进
 * @return HTTP_CODE
 */
Connection::HTTP_CODE Connection::_readProcess() {
    char* line;
    while (true)
    {
        HTTP_CODE ret;
        if(_checkState != CHECK_STATE::CHECK_BODY) {
            line = _getLine();
            if(line == nullptr) {
                return HTTP_CODE::NO_REQUEST;
            }
        }
        switch (_checkState)
        {
        case CHECK_STATE::CHECK_REQUEST_LINE:
            ret = _parseRequestLine(line);
            if(ret == HTTP_CODE::BAD_REQUEST) {
                return HTTP_CODE::BAD_REQUEST;
            }
            break;
        case CHECK_STATE::CHECK_HEADER:
            ret = _parseHeader(line);
            if(ret == HTTP_CODE::BAD_REQUEST) {
                return HTTP_CODE::BAD_REQUEST;
            }
            if(ret == HTTP_CODE::GET_REQUEST) {
                return _checkRequest();
            }
            break;
        case CHECK_STATE::CHECK_BODY:
            ret = _parseBody();
            if(ret == HTTP_CODE::BAD_REQUEST) {
                return HTTP_CODE::BAD_REQUEST;
            }
            if(ret == HTTP_CODE::NO_REQUEST) {
                return HTTP_CODE::NO_REQUEST;
            }
            if(ret == HTTP_CODE::GET_REQUEST) {
                return _checkRequest();
            }
            break;
        default:
            return HTTP_CODE::INTERNAL_ERROR;
            break;
        }
        printf("line:%s\n",line);
    }
    //return HTTP_CODE::NO_REQUEST;
}


void Connection::_writeProcess(HTTP_CODE ret) {
    switch (ret)
    {
    case HTTP_CODE::BAD_REQUEST:
        /* code */
        break;
    case HTTP_CODE::NO_RESOURCE:
        /* code */
        break;
    case HTTP_CODE::FORBIDDEN_REQUEST:
        /* code */
        break;
    case HTTP_CODE::FILE_REQUEST:
        _addStatusLine(ret);
        _addHeaders(_fileStat.st_size);
        _iov[0].iov_base = _writeBuff;
        _iov[0].iov_len = _writeBuffEnd;
        break;
    case HTTP_CODE::INTERNAL_ERROR:
    default:
        break;
    }
}
/**
 * @description: 写格式化字符串到writeBuff
 * @param {char*} format ...
 * @return 是否写成功
 */
bool Connection::_addResponse(const char* format, ...) {
    if(_writeBuffEnd >= BUFFERSIZE) {
        return false;
    }
    va_list vaList;
    va_start(vaList, format);
    //似乎没有vsnprintf_s的支持，只能认为buffer足够大，这里不会发生截断了
    int ret = vsnprintf(_writeBuff + _writeBuffEnd, BUFFERSIZE - _writeBuffEnd, format, vaList);
    if(ret < 0) {
        va_end(vaList);
        return false;
    }
    va_end(vaList);
    _writeBuffEnd += ret;
    return true;
}

bool Connection::_addStatusLine(HTTP_CODE ret) {
    switch (ret)
    {
    case HTTP_CODE::FILE_REQUEST:
        return _addResponse("HTTP/1.1 200 %s\r\n", ok_200_title);
        break;
        //TODO:
    default:
        break;
    }
    return true;
}

bool Connection::_addHeaders(int contentLen) {
    return  
    _addResponse("Content-Length:%d\r\n", contentLen) &&
    _addResponse("Connection:%s\r\n", (_keepAlive) ? "keep-alive" : "close") &&
    _addResponse("\r\n");
}

bool Connection::_addContent(const char* content) {
    return _addResponse("%s", content);
}
/**
 * @description: 解析 RequestLine
 * @param {char*} line
 * @return NO_REQUEST or BAD_REQUEST
 */
Connection::HTTP_CODE Connection::_parseRequestLine(char* line) {
    char method[10];
    int ret = sscanf(line, "%s %s %*s", method, _fileName);
    printf("_parseRequestLine: method->%s, filename->%s\n", method, _fileName);
    if(ret != 2) {
        return HTTP_CODE::BAD_REQUEST;
    }
    if(strcmp(method,"GET") == 0) {
        _method = METHOD::GET;
    }
    else if(strcmp(method,"POST") == 0) {
        _method = METHOD::POST;
    }
    else {
        //TODO:其他请求
        return HTTP_CODE::BAD_REQUEST;
    }
    _checkState = CHECK_STATE::CHECK_HEADER;
    return HTTP_CODE::NO_REQUEST;
}
/**
 * @description: 解析header
 * @param {char *} line
 * @return NO_REQUEST or GET_REQUEST or BAD_REQUEST
 */
Connection::HTTP_CODE Connection::_parseHeader(char * line) {
    assert(line != nullptr);
    if (*line == '\0')
    {
        if(_method == METHOD::GET) {
            return HTTP_CODE::GET_REQUEST;
        }
        _checkState = CHECK_STATE::CHECK_BODY;
        return HTTP_CODE::NO_REQUEST;
    }
    char key[100];
    char value[1024];
    for(size_t i=0;line[i] != '\0';++i) {
        if(isspace(line[i]) || line[i] == ':') {
            line [i] = '\0';
            strcpy(key, line);
            for(++i; line[i] != '\0' && !isgraph(line[i]); ++i) {}
            strcpy(value, line + i);
            break;
        }
    }
    if(strcasecmp(key, "connection") == 0) {
        if(strcasecmp(value, "close") == 0) {
            _keepAlive = false;
        }
        else {
            _keepAlive = true;
        }
    }
    if(strcasecmp(key, "content-length") == 0 && _method == METHOD::POST) {
        _contentLength = atoi(value);
    }
    //TODO::other headers
    return HTTP_CODE::NO_REQUEST;
}
/**
 * @description: 
 * @return GET_REQUEST, NO_REQUEST, BAD_REQUEST
 */
Connection::HTTP_CODE Connection::_parseBody() {
    int count = 0;
    for(;*(_readBuff + _readBufBegin + count) != '\0' && count < _contentLength; ++count) {}
    strncpy(_content + _contentIndex, _readBuff + _readBufBegin, count);
    _contentIndex += count;
    if(_contentIndex == _contentLength) {
        _content[_contentLength] = '\0';
        return HTTP_CODE::GET_REQUEST;
    }
    return HTTP_CODE::NO_REQUEST;
}
/**
 * @description: 根据文件是否存在，是否出错返回HTTP_CODE
 * @return HTTP_CODE
 */    
Connection::HTTP_CODE Connection::_checkRequest() {
    char* p = strrchr(_fileName, '/');
    if(p == nullptr) {
        return HTTP_CODE::BAD_REQUEST;
    }
    char path[100];
    strcpy(path, _path);
    strcat(path, p+1);
    int ret = stat(path, &_fileStat);
    if(ret == -1) {
        return HTTP_CODE::NO_RESOURCE; 
    }
    if(!(_fileStat.st_mode & S_IROTH)) {
        return HTTP_CODE::FORBIDDEN_REQUEST;
    }
    if(S_ISDIR(_fileStat.st_mode)) {
        return HTTP_CODE::BAD_REQUEST;
    }
    int fd = open(path, O_RDONLY);
    assert(fd > 0);
    _fileAddress = (char*)mmap(NULL, _fileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    _iov[1].iov_base = _fileAddress;
    _iov[1].iov_len = _fileStat.st_size;
    assert((void *)_iov[1].iov_base != (void *) -1);
    return HTTP_CODE::FILE_REQUEST;
}
/**
 * @description: 
 * @return 1:保留Connection, -1:删除Connection
 */
int Connection::readEvent() {
    int ret1 = readOnce();
    printf("readOnce:%s\n", _readBuff);
    if(ret1 == -1) {
        return -1;
    }
    HTTP_CODE ret = _readProcess();
    if(ret == HTTP_CODE::NO_REQUEST) {
        _reorganizeBuff();
        struct epoll_event event;
        event.data.fd = _fd;
        event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLET | EPOLLONESHOT;
        int ret = epoll_ctl(epollFd, EPOLL_CTL_MOD, _fd, &event);
        assert(ret != -1);
    }
    else {
        _writeProcess(ret);
        printf("%s\n", _writeBuff);
        struct epoll_event event;
        event.data.fd = _fd;
        event.events = EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLET | EPOLLONESHOT;
        int ret = epoll_ctl(epollFd, EPOLL_CTL_MOD, _fd, &event);
        printf("ret:%d\n", ret);
        assert(ret != -1);
    }
    return 1;
}
/**
 * @description: 
 * @return 1:保留Connection, -1:删除Connection
 */
int Connection::writeEvent() {
    int ret = writeOnce();
    printf("writeEvent ret:%d\n", ret);
    if(ret == 1) {
        if(_keepAlive) {
            printf("keepAlive\n");
            _init();
            struct epoll_event event;
            event.data.fd = _fd;
            event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLET | EPOLLONESHOT;
            int ret = epoll_ctl(epollFd, EPOLL_CTL_MOD, _fd, &event);
            assert(ret != -1);
            return 1;
        }
        else {
            printf("nonkeepAlive\n");
            return -1;
        }
    }
    if(ret == -1) {
        struct epoll_event event;
        event.data.fd = _fd;
        event.events = EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLET | EPOLLONESHOT;
        int ret = epoll_ctl(epollFd, EPOLL_CTL_MOD, _fd, &event);
        assert(ret != -1);
        return 1;
    }
    if (ret == -2) {
        return -1;
    }
}

void Connection::_reorganizeBuff() {
    if(_readBufBegin == 0) return ;
    if(_readBufBegin >= _readBuffEnd) {
        _readBufBegin = _readBuffEnd = 0;
        return ;
    }
    for(size_t i = _readBufBegin ;_readBuff[i] != '\0'; ++i) {
        _readBuff[i - _readBufBegin] = _readBuff[i];
    }
    _readBuffEnd -= _readBufBegin;
    _readBufBegin = 0;
}