#pragma once
#include <queue>
#include <unordered_map>
#include <time.h> 
#include <algorithm>
#include <arpa/inet.h> //?
#include <functional>
#include <assert.h>
#include <chrono>
#include "../log/log.h"

using TimeoutCallBack = std::function<void()>;
using MS = std::chrono::milliseconds;
using Clock = std::chrono::system_clock;
using TimeStamp = Clock::time_point;

struct TimeNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimeNode& t) {
        return expires < t.expires;
    }
    bool operator<=(const TimeNode& t) {
        return expires <= t.expires;
    }
    TimeNode() = default;
    TimeNode(const TimeNode& t) = default;
    TimeNode& operator=(const TimeNode& t) = default;
    TimeNode(int i, TimeStamp exp, TimeoutCallBack c):id(i),expires(exp), cb(c){
    }
};

class HeapTimer {
public:
    HeapTimer():_heap(64) {}
    ~HeapTimer(){clear();}
    void clear();
    void add(int id, int timeout, const TimeoutCallBack& cb);
    void doWork(int id);
    void tick();
    void pop();
    int getNextTick();
    void adjust(int id, int timeoutMS);
private:
    void _del(size_t i);
    void _siftUp(size_t i);
    bool _siftDown(size_t i);
    void _swapNode(size_t i, size_t j);
    void _adjust(size_t i);
    std::vector<TimeNode> _heap;
    std::unordered_map<int, size_t> _ref;
};