#pragma once
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <functional>
#include <shared_mutex>
#include <iostream>
#include "Logger.hpp"
#include "ProcessMonitor.hpp"

class Task
{
public:
	std::string destination;
	std::string source;
	int flag;
	int fd;

	Task(std::string destination, std::string source, int flag, int fd)
	{
		this->destination = destination;
		this->source = source;
		this->flag = flag;
		this->fd = fd;
	};

	Task() {
	};

	~Task() {
	};
};

template <typename T = Task>
class Channel
{
private:
	std::queue<T> queue;
	std::mutex mutex;
	std::condition_variable not_empty;
	bool isTerminated;
	static const int MAX_ITEM_QUEUED = 10000;

public:
	Channel()
	{
		// Initialize queue
		this->queue = std::queue<T>();
		this->isTerminated = false;
	}

	void send(const T &value)
	{
		// Get exclusive access to the queue
		std::unique_lock<std::mutex> l(this->mutex);
		if (this->queue.size() >= MAX_ITEM_QUEUED)
		{
			Logger::add_logs("Queue is full, can't add more task.", LogLevel::WARNING);
			return;
		}

		// Put value into queue
		this->queue.push(value);

		// Notify receivers
		this->not_empty.notify_one();
	}

	void sendStop()
	{
		this->isTerminated = true;
		Logger::add_logs("All the thread were ordered to die.", LogLevel::WARNING);
		this->not_empty.notify_all();
	}

	bool recv(T &value)
	{
		// Get exclusive access to the queue when it's not empty
		std::unique_lock<std::mutex> l(this->mutex);
		this->not_empty.wait(l, [this]
							 { return !this->queue.empty() || this->isTerminated; });
		if (this->isTerminated)
		{
			return false;
		}
		else
		{
			value = this->queue.front();
			this->queue.pop();
			return true;
		}
	}
};
template <typename T = Task>
class Worker
{
private:
	std::shared_ptr<Channel<T>> chan;
	std::thread thread;

public:
	bool isRunning;
	uint threadID;
	static uint numThread;

	Worker(std::shared_ptr<Channel<T>> &chan, void (*asyncFunction)(T task))
	{
		this->chan = chan;
		this->threadID = numThread + 1;
		this->numThread += 1;
		this->isRunning = false;
		this->thread = std::thread([=]()
								   {
			for (;;)
			{
				T value;
				this->isRunning = false;
				if(!chan->recv(value))
				{
					break;
				}
				this->isRunning = true;
				ProcessMonitor pm("Worker ");
				asyncFunction(value);
			} });
	}

	~Worker()
	{
		if (this->thread.joinable())
		{
			this->thread.join();
		}
		this->numThread -= 1;
		Logger::add_logs("Worker " + std::to_string(this->threadID) + " is destroyed.", LogLevel::WARNING);
		if (this->numThread == 0)
		{
			Logger::add_logs("All workers are destroyed.", LogLevel::WARNING);
		}
	}
};
template <typename T>
uint Worker<T>::numThread = 0;

template <typename T, int N>
class ThreadPool
{

private:
	std::array<std::unique_ptr<Worker<T>>, N> pool;
	std::shared_ptr<Channel<T>> chan;

public:
	ThreadPool(void (*asyncFunction)(T task))
	{
		this->chan = std::make_shared<Channel<T>>();
		for (auto &worker : this->pool)
		{
			worker = std::make_unique<Worker<T>>(chan, asyncFunction);
		}
	}
	~ThreadPool()
	{
		this->chan->sendStop();
	}

	void send(const T &task)
	{
		this->chan->send(task);
	}

	int get_running_workers()
	{
		int runningThread = 0;
		for (auto &worker : pool)
		{
			if (worker->isRunning)
			{
				runningThread += 1;
			}
		}
		return runningThread;
	}

	int get_max_workers()
	{
		return Worker<T>::numThread;
	}
};
