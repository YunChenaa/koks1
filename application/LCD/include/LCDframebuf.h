#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <cstdint>

class LCDFrameBuffer
{
public:
    LCDFrameBuffer(size_t width, size_t height, size_t bufferCount )
        : m_width(width), m_height(height)
    {
        m_buffers.resize(bufferCount);
        for (auto& buf : m_buffers) {
            buf.resize(width * height * 3 / 2); // nv12格式每个像素占1.5字节
        }
        m_writeIndex = 0;
        m_readIndex = 0;
        m_count = 0;
    }

    // 写入数据到队列尾部（队列满时阻塞直到有空位置）
    bool write(const std::vector<uint8_t>& data)
    {
        if (data.size() != m_width * m_height * 3 / 2) return false;
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv_write.wait(lock, [this] { return m_count < m_buffers.size(); });
        m_buffers[m_writeIndex] = data;
        m_writeIndex = (m_writeIndex + 1) % m_buffers.size();
        ++m_count;
        m_cv_read.notify_one();
        return true;
    }

    // 从队列头部读取数据（队列空时阻塞直到有数据）
    bool read(std::vector<uint8_t>& out)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv_read.wait(lock, [this] { return m_count > 0; });
        out = m_buffers[m_readIndex];
        m_readIndex = (m_readIndex + 1) % m_buffers.size();
        --m_count;
        m_cv_write.notify_one();
        return true;
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // for (auto& buf : m_buffers) {
        //     buf.clear();
        //     buf.resize(m_width * m_height * 3 / 2);
        // }
        
        m_writeIndex = 0;
        m_readIndex = 0;
        m_count = 0;
    }

private:
    size_t m_width;
    size_t m_height;
    std::vector<std::vector<uint8_t>> m_buffers;
    size_t m_writeIndex;
    size_t m_readIndex;
    size_t m_count;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv_write;
    std::condition_variable m_cv_read;
};
