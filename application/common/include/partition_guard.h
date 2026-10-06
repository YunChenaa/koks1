#pragma once
#include <mutex>
#include <condition_variable> 

namespace st
{
namespace sense_koks
{
enum ConfigFileFlag {
    CONFIG_FILE_UNKNOWN = 0, 
    CONFIG_CONFIG_JSON = (1 << 0),
    CONFIG_FACE_DB = (1 << 1),
    CONFIG_PALM_DB = (1 << 2),
    CONFIG_FILE_ALL = 0xFFFF,
};

class PartitionGuard
{
    public:
    static void Init();

    PartitionGuard(int flags, bool waitForOthers = true);
    ~PartitionGuard();

    PartitionGuard(const PartitionGuard&) = delete;
    PartitionGuard& operator=(const PartitionGuard&) = delete;

    void SyncAndLock();

    private:
    enum State { NONE, LOCK, UNLOCK };

    static std::mutex s_counter_mutex; // 保护计数器
    static std::condition_variable s_cv;
    static int s_refcount;             // 引用计数

    void release();
    void SetState(State state);
    State mState;
    int mFileFlags;
    bool m_waitForOthers;
    bool m_deinitialized = false;
};

} // namespace sense_koks

} // namespace st
