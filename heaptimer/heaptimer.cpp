#include "heaptimer.h"

void HeapTimer::_siftUp(size_t i) {
    assert(i>=0&&i<_heap.size());
    size_t fa = (i-1)>>1;
    while(fa >= 0) {
        if(_heap[fa] <= _heap[i]) break;
        _swapNode(i, fa);
        i = fa;
        fa = (i-1) >> 1;
    }
}

void HeapTimer::_swapNode(size_t i, size_t j) {
    assert(i>=0&&i<_heap.size());
    assert(j>=0&&j<_heap.size());
    std::swap(_heap[i], _heap[j]);
    _ref[_heap[i].id] = i;
    _ref[_heap[j].id] = j;
}

bool HeapTimer::_siftDown(size_t index){
    assert(index>=0&&index<_heap.size());
    size_t i = index;
    size_t j = i*2 + 1;
    while(j < _heap.size()) {
        if(_heap[i] <= _heap[j]) break;
        _swapNode(i,j);
        i = j;
        j = i*2 + 1;
    }
    return i > index;
} 
void HeapTimer::_adjust(size_t index) {
    if(!_siftDown(index)) {
        _siftUp(index);
    }
}

void HeapTimer::adjust(int id, int timeoutMS) {
    assert(id);
    if(_ref.count(id)==1){
        size_t idx = _ref[id];
        _heap[idx].expires = Clock::now() + MS(timeoutMS);
        _adjust(idx);
    }
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    assert(id >= 0);
    if(_ref.count(id)==0) {
        _ref[id] = _heap.size();
        _heap.emplace_back(id, Clock::now() + MS(timeout), cb);
        _siftUp(_heap.size()-1);
    }
    else {
        size_t i = _ref[id];
        _heap[i].expires = Clock::now() + MS(timeout);
        _heap[i].cb = cb;
        _adjust(i);
    }
}

void HeapTimer::doWork(int id) {
    if(_heap.empty() || _ref.count(id)==0) {
        return ;
    }
    size_t i = _ref[id];
    _heap[i].cb();
    _del(i);
}

void HeapTimer::_del(size_t index) {
    assert(!_heap.empty()&&index>=0&&index<_heap.size());
    if(index < _heap.size()-1) {
        _swapNode(index, _heap.size()-1);
        _adjust(index);
    }
    _ref.erase(_heap.back().id);
    _heap.pop_back();
}

void HeapTimer::tick() {
    if(_heap.empty()) return ;
    while(!_heap.empty()) {
        if(std::chrono::duration_cast<MS>(_heap.front().expires - Clock::now()).count() > 0) { //超时时间大于现在，表示现在还未超时
            break;
        }
        _heap.front().cb();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!_heap.empty());
    _del(0u);
}

void HeapTimer::clear() {
    _heap.clear();
    _ref.clear();
}

int HeapTimer::getNextTick() {
    tick();
    int res = -1;
    if(!_heap.empty()) {
        res = std::chrono::duration_cast<MS>(_heap.front().expires - Clock::now()).count();
        if(res < 0) {
            res = 0;
        }
    }
    return res;
}