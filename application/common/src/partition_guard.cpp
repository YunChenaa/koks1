#include "partition_guard.h"
#include "os_utils.h"
#include "config_bk.h"
#include "koks_project_def.h"
#include "sys_logger.h"
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <sys/mount.h>
#include <unistd.h>

namespace st
{
namespace sense_koks
{

std::mutex PartitionGuard::s_counter_mutex;
std::condition_variable PartitionGuard::s_cv;
int PartitionGuard::s_refcount = 0;

static const char* GetFileName(int flag)
{
    switch (flag) {
    case CONFIG_CONFIG_JSON:
        return "config.json";
#ifdef KOKS_FEATURE_PALM
    case CONFIG_FACE_DB:
        return "db/face/userlist.bin";
    case CONFIG_PALM_DB:
        return "db/palm/userlist.bin";
#else
    case CONFIG_FACE_DB:
        return "db/userlist.bin";
#endif
    default:
        break;
    }
    return NULL;
}

void PartitionGuard::Init()
{
    config_bk_init();
#ifdef KOKS_FEATURE_PALM
    int config_files[] = { CONFIG_CONFIG_JSON, CONFIG_FACE_DB, CONFIG_PALM_DB};
#else
    int config_files[] = { CONFIG_CONFIG_JSON, CONFIG_FACE_DB };
#endif
    int file_cnt = sizeof(config_files) / sizeof(config_files[0]);
    for (int i = 0; i < file_cnt; ++i) {
        config_bk_check_file(GetFileName(config_files[i]));
    }
}

PartitionGuard::PartitionGuard(int fileFlags, bool waitForOthers)
    : mFileFlags(fileFlags), m_waitForOthers(waitForOthers)
{
    std::unique_lock<std::mutex> lock(s_counter_mutex);
    if (s_refcount == 0) {
        SetState(State::UNLOCK);
    }
    s_refcount++;
}

PartitionGuard::~PartitionGuard()
{
    release();
}

void PartitionGuard::release()
{
    if (m_deinitialized) return;
    m_deinitialized = true;

    std::unique_lock<std::mutex> lock(s_counter_mutex);

    if (m_waitForOthers) {
        // 还有其它实例，继续等待
        while (s_refcount > 1) {
            s_cv.wait(lock);
        }

        s_refcount--;
        SetState(State::LOCK);
    } else {
        s_refcount--;
        if (s_refcount == 0) {
            SetState(State::LOCK);
        }
        s_cv.notify_all();
    }
}

void PartitionGuard::SyncAndLock()
{
    release();
}

void PartitionGuard::SetState(State state)
{
    if (state == State::LOCK) {
        if (mFileFlags & CONFIG_CONFIG_JSON) {
            config_bk_update(GetFileName(CONFIG_CONFIG_JSON), false);
        }
        if (mFileFlags & CONFIG_FACE_DB) {
            config_bk_update(GetFileName(CONFIG_FACE_DB), false);
        }
#ifdef KOKS_FEATURE_PALM
        if (mFileFlags & CONFIG_PALM_DB) {
            config_bk_update(GetFileName(CONFIG_PALM_DB), false);
        }
#endif
    }

    config_bk_remount(PARTITION_CONFIG, (state == State::LOCK));
}

} // namespace sense_koks

} // namespace st
