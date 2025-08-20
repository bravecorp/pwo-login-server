/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef CORE_TASKS_H
#define CORE_TASKS_H

#include <condition_variable>
#include <functional>
#include <chrono>

#include <core/threadholder.h>

using TaskFunc = std::function<void(void)>;
const int DISPATCHER_TASK_EXPIRATION = 2000;
const auto SYSTEM_TIME_ZERO = std::chrono::system_clock::time_point(std::chrono::milliseconds(0));

class Task
{
public:
	// DO NOT allocate this class on the stack
	explicit Task(TaskFunc&& f) : m_func(std::move(f)) {}
	Task(uint32_t ms, TaskFunc&& f) :
		m_expiration(std::chrono::system_clock::now() + std::chrono::milliseconds(ms)), m_func(std::move(f)) {
	}

	virtual ~Task() = default;
	void operator()() {
		m_func();
	}

	void setDontExpire() {
		m_expiration = SYSTEM_TIME_ZERO;
	}

	bool hasExpired() const {
		if (m_expiration == SYSTEM_TIME_ZERO) {
			return false;
		}
		return m_expiration < std::chrono::system_clock::now();
	}

protected:
	std::chrono::system_clock::time_point m_expiration = SYSTEM_TIME_ZERO;

private:
	// Expiration has another meaning for scheduler tasks,
	// then it is the time the task should be added to the
	// dispatcher
	TaskFunc m_func;
};

Task* createTask(TaskFunc&& f);
Task* createTask(uint32_t expiration, TaskFunc&& f);

class Dispatcher : public ThreadHolder<Dispatcher> {
public:
	void addTask(Task* task);

	void shutdown();

	uint64_t getDispatcherCycle() const {
		return m_dispatcherCycle;
	}

	void threadMain();

private:
	std::mutex m_taskLock;
	std::condition_variable m_taskSignal;

	std::vector<Task*> m_taskList;
	uint64_t m_dispatcherCycle = 0;
};

extern Dispatcher g_dispatcher;

#endif
