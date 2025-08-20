/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef CORE_THREAD_HOLDER_H
#define CORE_THREAD_HOLDER_H

#include <thread>
#include <atomic>

template <typename Derived>
class ThreadHolder {
    public:
        enum class ThreadState {
            Running,
            Closing,
            Terminated,
        };

        ThreadHolder() = default;
        ~ThreadHolder() = default;

        // non-copyable
        ThreadHolder(const ThreadHolder&) = delete;
        ThreadHolder& operator=(const ThreadHolder&) = delete;

        void start() {
            setState(ThreadState::Running);
            m_thread = std::thread(&Derived::threadMain, static_cast<Derived*>(this));
        }

        void stop() {
            setState(ThreadState::Closing);
        }

        void join() {
            if (m_thread.joinable()) {
                m_thread.join();
            }
        }

    protected:
        void setState(ThreadState newState) {
            m_threadState.store(newState, std::memory_order_relaxed);
        }

        ThreadState getState() const {
            return m_threadState.load(std::memory_order_relaxed);
        }

    private:
        std::atomic<ThreadState> m_threadState{ThreadState::Terminated};
        std::thread m_thread;
};

#endif
