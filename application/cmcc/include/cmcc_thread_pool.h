#pragma once

#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

#define QUEUE_MAX 100

template<typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(size_t max_size) : max_size_(max_size) {}

    void push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock, [this] { return queue_.size() < max_size_ || aborted_; });
        if (aborted_) return;
        queue_.push(std::move(item));
        not_empty_.notify_one();
    }
    
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this] { return !queue_.empty() || aborted_; });
        if (aborted_ && queue_.empty()) return T{}; 
        T item = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return item;
    }
    
    void abort() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            aborted_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::queue<T> empty;
        std::swap(queue_, empty);
        not_full_.notify_all();
    }
    
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    size_t max_size_;
    bool aborted_ = false;
};

class ManagedThread{
public:
    using ThreadFunc = std::function<void(std::atomic<bool>&)>;

    ManagedThread() = default;

    explicit ManagedThread(ThreadFunc func)
        : m_func(std::move(func)), m_stop(false) {}

    ManagedThread(ManagedThread&& other) noexcept
        : m_func(std::move(other.m_func)),
          m_stop(false),
          m_thread(std::move(other.m_thread)),
          m_detached(other.m_detached) {
        other.m_detached = false;
    }

    ManagedThread& operator=(ManagedThread&& other) noexcept {
        if (this != &other) {
            stop();
            m_func = std::move(other.m_func);
            m_thread = std::move(other.m_thread);
            m_detached = other.m_detached;
            other.m_detached = false;
        }
        return *this;
    }

    ManagedThread(const ManagedThread&) = delete;
    ManagedThread& operator=(const ManagedThread&) = delete;

    ~ManagedThread() {
        stop();
    }

    static void setBlockStart(bool block) {
        s_blockStart = block;
    }
    static bool isBlockStart() {
        return s_blockStart;
    }

    void reset() {
        m_stop = false;
    }

    void start() {
        if (s_blockStart) {
            return;
        }
        if (!m_thread.joinable() && !m_detached) {
            m_detached = false;
            m_thread = std::thread([this]() {
                m_func(m_stop);
            });
        }
    }

    void startDetached() {
        if (s_blockStart) {
            return;
        }
        if (!m_thread.joinable() && !m_detached) {
            m_detached = true;
            m_thread = std::thread([this]() {
                m_func(m_stop);
            });
            m_thread.detach();
        }
    }

    void stop() {
        m_stop = true;
        if (!m_detached && m_thread.joinable()) {
            m_thread.join();
        }
    }

    bool isRunning() const {
        return !m_detached && m_thread.joinable() && !m_stop;
    }

    bool isDetached() const {
        return m_detached;
    }

private:
    ThreadFunc m_func;
    std::atomic<bool> m_stop{false};
    std::thread m_thread;
    bool m_detached = false;
    static std::atomic<bool> s_blockStart;
};

class EventThread{
public:
    using ThreadFunc = std::function<void(std::atomic<bool>&)>;

    EventThread() = default;
    explicit EventThread(ThreadFunc func) : m_func(std::move(func)) {}

    ~EventThread() { stop(); }

    static void setBlockStart(bool block) {
        s_blockStart = block;
    }
    static bool isBlockStart() {
        return s_blockStart;
    }

    void reset() {
        m_stop = false;
    }

    void start() {
        if (s_blockStart) {
            return;
        }
        if (!m_thread.joinable()) {
            m_stop = false;
            m_thread = std::thread([this]() { threadLoop(); });
        }
    }

    void startDetached() {
        if (s_blockStart) {
            return;
        }
        if (!m_thread.joinable()) {
            m_thread = std::thread([this]() { threadLoop(); });
            m_thread.detach();
        }
    }

    void stop() {
        m_stop = true;
        m_cv.notify_all();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    void notify() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pending = true;
        }
        m_cv.notify_one();
    }

    bool isRunning() const { return !m_stop && m_thread.joinable(); }

private:
    void threadLoop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_stop) {
            m_cv.wait(lock, [this] { return m_pending || m_stop; });
            if (m_stop) break;
            m_pending = false;
            lock.unlock();
            if (m_func) m_func(m_stop);
            lock.lock();
        }
    }

    ThreadFunc m_func;
    std::atomic<bool> m_stop{false};
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_pending = false;
    static std::atomic<bool> s_blockStart;
};

extern std::unique_ptr<EventThread> g_monitorThread;
extern BlockingQueue<std::vector<uint8_t>> audio_queue;
extern BlockingQueue<std::vector<uint8_t>> video_queue;