/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-01-07 16:09:05
 * @LastEditors: 洪照
 * @LastEditTime: 2025-08-14 17:51:31
 * @FilePath: /sense-koks/application/db/include/st_koks_db.h
 */
#pragma once
#include "koks_project_def.h"
#include "json11.hpp"
#include "message_v2.h"
#include "st_koks_errno.h"
#include "sys_logger.h"
#include <mutex>
#include <string>

namespace st
{
namespace sense_koks
{

// CRTP 基类
template <typename T>
class DataStrategy
{
    public:
    ~DataStrategy() = default;
    ST_ERR read(const std::string& filename, void* data)
    {
        return static_cast<T*>(this)->readImpl(filename, data);
    }
    ST_ERR write(const std::string& filename, const void* data)
    {
        return static_cast<T*>(this)->writeImpl(filename, data);
    }
};

// 具体策略类
class SnStrategy : public DataStrategy<SnStrategy>
{
    public:
    ST_ERR readImpl(const std::string& filename, void* data);
    ST_ERR writeImpl(const std::string& filename, const void* data);
};

class ThresholdLevelStrategy : public DataStrategy<ThresholdLevelStrategy>
{
    public:
    ST_ERR readImpl(const std::string& filename, void* data);
    ST_ERR writeImpl(const std::string& filename, const void* data);
};

class ThresholdStrategy : public DataStrategy<ThresholdStrategy>
{
    public:
    ST_ERR readImpl(const std::string& filename, void* data);
    ST_ERR writeImpl(const std::string& filename, const void* data);
};

class KeyStrategy : public DataStrategy<KeyStrategy>
{
    public:
    ST_ERR readImpl(const std::string& filename, void* data);
    ST_ERR writeImpl(const std::string& filename, const void* data);
};

class AimiscStrategy : public DataStrategy<AimiscStrategy>
{
    public:
    ST_ERR readImpl(const std::string& filename, void* data);
    ST_ERR writeImpl(const std::string& filename, const void* data);
};

class BlurStrategy : public DataStrategy<BlurStrategy>
{
    public:
    ST_ERR readImpl(const std::string& filename, void* data);
    ST_ERR writeImpl(const std::string& filename, const void* data);
};
#ifdef KOKS_FEATURE_LCD_EMOTION

class LcdEmotionStrategy : public DataStrategy<LcdEmotionStrategy>
{
    public:
    ST_ERR readImpl(const std::string& filename, void* data);
    ST_ERR writeImpl(const std::string& filename, const void* data);
};
#endif


#ifdef KOKS_FEATURE_PALM
class PalmStrategy : public DataStrategy<PalmStrategy>
{
    public:
    ST_ERR readImpl(const std::string& filename, void* data);
    ST_ERR writeImpl(const std::string& filename, const void* data);
};
#endif

class koks_db
{
    public:
    static koks_db& GetInstance(void);
    ST_ERR Init(const std::string& path);
    void DeInit(void);
    template <typename T>
    ST_ERR readData(T* strategy, void* data)
    {
        std::lock_guard<std::mutex> lock(db_mutex);
        return strategy->read(db_path, data);
    }
    template <typename T>
    ST_ERR writeData(T* strategy, const void* data)
    {
        std::lock_guard<std::mutex> lock(db_mutex);
        auto result = strategy->write(db_path, data);
        return result;
    }

    private:
    koks_db();
    ~koks_db();
    koks_db(const koks_db&) = delete;
    koks_db& operator=(const koks_db&) = delete;
    std::string db_path;
    std::mutex db_mutex;
};

} // namespace sense_koks
} // namespace st
