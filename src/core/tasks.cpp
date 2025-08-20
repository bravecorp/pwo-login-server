/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include <core/tasks.h>

Dispatcher g_dispatcher;

Task* createTask(TaskFunc&& f)
{
	return new Task(std::move(f));
}

Task* createTask(uint32_t expiration, TaskFunc&& f)
{
	return new Task(expiration, std::move(f));
}

void Dispatcher::threadMain()
{
	std::vector<Task*> tmpTaskList;
	// NOTE: second argument defer_lock is to prevent from immediate locking
	std::unique_lock<std::mutex> taskLockUnique(m_taskLock, std::defer_lock);

	while (getState() != ThreadState::Terminated) {
		// check if there are tasks waiting
		taskLockUnique.lock();
		if (m_taskList.empty()) {
			//if the list is empty wait for signal
			m_taskSignal.wait(taskLockUnique);
		}
		tmpTaskList.swap(m_taskList);
		taskLockUnique.unlock();

		for (Task* task : tmpTaskList) {
			if (!task->hasExpired()) {
				++m_dispatcherCycle;
				// execute it
				(*task)();
			}
			delete task;
		}
		tmpTaskList.clear();
	}
}

void Dispatcher::addTask(Task* task)
{
	bool do_signal = false;

	m_taskLock.lock();

	if (getState() == ThreadState::Running) {
		do_signal = m_taskList.empty();
		m_taskList.push_back(task);
	} else {
		delete task;
	}

	m_taskLock.unlock();

	// send a signal if the list was empty
	if (do_signal) {
		m_taskSignal.notify_one();
	}
}

void Dispatcher::shutdown()
{
	Task* task = createTask([this]() {
		setState(ThreadState::Terminated);
		m_taskSignal.notify_one();
		});

	std::lock_guard<std::mutex> lockClass(m_taskLock);
	m_taskList.push_back(task);

	m_taskSignal.notify_one();
}
