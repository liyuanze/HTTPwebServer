/*
 * @Author: liyuanze
 * @Date: 2022-08-15 23:37:36
 * @FilePath: /myTinyWebServer/code/threadPool/threadPool.h
 * Copyright (c) 2022 by liyuanze, All Rights Reserved. 
 */

#pragma once
#include <queue>
#include <functional>
#include <vector>
#include <thread>
#include <iostream>
#include <mutex>
#include <memory>
#include <future>
class threadPool {
public:
	threadPool(int numThreads=4):_numThreads(numThreads),_isShutdown(false) {_init();}
	threadPool(const threadPool&) = delete;
	threadPool(threadPool&&) = delete;
	threadPool& operator =(const threadPool&) = delete;
	threadPool& operator =(threadPool&&) = delete;
	void shotdown() {
		_isShutdown = true;
		_cv.notify_all();
		for (std::thread& item : _threadPool) {
			if (item.joinable()) {
				item.join();
			}
		}
	}
	template <typename T, typename... Args>
	auto submit(T&& f, Args&& ... args) {
		using retType = decltype(f(args...));
		std::function<retType()> func = std::bind(std::forward<T>(f), std::forward<Args>(args)...);
		auto task_ptr = std::make_shared<std::packaged_task<retType()>>(func);
		std::function<void()> newFunc = [task_ptr]() {(*task_ptr)(); };
		std::unique_lock<std::mutex> lc(_mux);
		_funcQueue.push(newFunc);
		lc.unlock();
		_cv.notify_one();
		return (*task_ptr).get_future();
	}
private:
	class threadWorker {
	public:
		threadWorker(int id,threadPool* pool):_id(id),_pool(pool) {}
		void operator()() {
			while (true) {
				std::unique_lock<std::mutex> lc(_pool->_mux);
				if(_pool->_funcQueue.empty())
					_pool->_cv.wait(lc);
				if (_pool->_isShutdown) break;
				std::function<void()> _func = _pool->_funcQueue.front();
				_pool->_funcQueue.pop();
				lc.unlock();
				_func();
			}
		}
	private:
		int _id;
		threadPool* _pool;
		
	};
	std::queue<std::function<void()>> _funcQueue;
	std::vector<std::thread> _threadPool;
	std::mutex _mux;
	std::condition_variable _cv;
	bool _isShutdown;
	size_t _numThreads;

	void _init() {
		for (size_t i = 0; i < _numThreads; ++i) {
			_threadPool.emplace_back(threadWorker(i,this));
		}
	}

};