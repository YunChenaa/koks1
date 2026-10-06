/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-01-07 16:09:13
 * @LastEditors: 洪照
 * @LastEditTime: 2025-08-14 18:24:00
 * @FilePath: /sense-koks/application/db/src/st_koks_db.cpp
 */
#include "st_koks_db.h"
#include "st_koks_errno.h"
#include "sys_logger.h"
#include "partition_guard.h"
#include <cstdint>
#ifdef KOKS_FEATURE_PALM
#include "st_palm_module.h"
#endif
#include "json11.hpp"
#include <fstream>
#include <sstream>
#include <vector>
namespace st
{
namespace sense_koks
{

static ST_ERR koks_db_read_file(const std::string& filename, std::stringstream& buffer, json11::Json& json)
{
    const std::string backup_file = "/etc/koks/config.json";
    std::vector<std::string> files_to_try = { filename, backup_file };

    for (size_t i = 0; i < files_to_try.size(); ++i) {
        const std::string& current_file = files_to_try[i];
        bool is_backup = (i > 0);

        std::ifstream file(current_file);
        if (!file.is_open()) {
            LOG_ERROR("%s open failed!\n", current_file.c_str());
            if (is_backup) {
                return ST_ERRNO_FAIL;
            }
            continue;
        }

        if (is_backup) {
            LOG_INFO("Using backup file: %s\n", current_file.c_str());
        }

        buffer.str("");
        buffer.clear();
        buffer << file.rdbuf();
        file.close();

        std::string err;
        json = json11::Json::parse(buffer.str(), err);
        if (err.empty()) {
            return ST_ERRNO_SUCCESS;
        }

        LOG_ERROR("%s JSON parse failed: %s\n", current_file.c_str(), err.c_str());
        if (is_backup) {
            return ST_ERRNO_FAIL;
        }
    }

    return ST_ERRNO_SUCCESS;
}

static ST_ERR koks_db_write_file(const std::string& filename, const std::string& new_content, const std::string& old_content)
{
    if (old_content == new_content) {
        return ST_ERRNO_SUCCESS;
    }

    auto tmpfile = filename + ".tmp";
    ST_ERR ret = ST_ERRNO_FAIL;
    PartitionGuard configGuard(CONFIG_CONFIG_JSON);
    FILE* fp = fopen(tmpfile.c_str(), "w");
    do {
        if (!fp) {
            LOG_ERROR("open file failed: %s\n", tmpfile.c_str());
            break;
        }

        size_t written = fwrite(new_content.c_str(), 1, new_content.size(), fp);
        if (written != new_content.size()) {
            LOG_ERROR("write file failed: %s\n", tmpfile.c_str());
            break;
        }

        if (fflush(fp) != 0 || fsync(fileno(fp)) != 0) {
            break;
        }

        if (fclose(fp) != 0) {
            break;
        }
        fp = NULL;

        if (rename(tmpfile.c_str(), filename.c_str()) != 0) {
            LOG_ERROR("rename file failed: %s\n", tmpfile.c_str());
            break;
        }
        LOG_INFO("write file OK: %s\n", filename.c_str());
        ret = ST_ERRNO_SUCCESS;
    } while (0);

    if (fp) {
        fclose(fp);
    }
    if (ret != ST_ERRNO_SUCCESS) {
        unlink(tmpfile.c_str());
    }
    configGuard.SyncAndLock();
    return ret;
}

/**
 * @name: koks_db
 * @brief: constructor
 * @return {*}
 */
koks_db::koks_db()
{
}

/**
 * @name: ~koks_db
 * @brief: destructor
 * @return {*}
 */
koks_db::~koks_db()
{
}

/**
 * @name: GetInstance
 * @brief: 获取单例
 * @return {*}
 */
koks_db& koks_db::GetInstance(void)
{
    static koks_db instance;
    return instance;
}

/**
 * @name: Init
 * @brief: 初始化数据库
 * @param {const std::string&} path 数据库路径
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR koks_db::Init(const std::string& path)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    db_path = path;
    LOG_DEBUG("Opened database successfully\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: DeInit
 * @brief: 反初始化
 * @return {*}
 */
void koks_db::DeInit(void)
{
    LOG_DEBUG("Closed database successfully\n");
}

// SnStrategy 实现
ST_ERR SnStrategy::readImpl(const std::string& filename, void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    std::string* sn = static_cast<std::string*>(data);
    *sn = json["sn"].string_value();

    return ST_ERRNO_SUCCESS;
}

ST_ERR SnStrategy::writeImpl(const std::string& filename, const void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    json11::Json::object json_obj = json.object_items();
    const std::string* sn = static_cast<const std::string*>(data);
    json_obj["sn"] = *sn;

    return koks_db_write_file(filename, json11::Json(json_obj).dump(), buffer.str());
}

// ThresholdLevelStrategy 实现
ST_ERR ThresholdLevelStrategy::readImpl(const std::string& filename, void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    s_msg_algo_threshold_level* level = static_cast<s_msg_algo_threshold_level*>(data);
    level->verify_threshold_level = static_cast<uint8_t>(json["threshold_level"]["verify_threshold_level"].int_value());
    level->liveness_threshold_level = static_cast<uint8_t>(json["threshold_level"]["liveness_threshold_level"].int_value());

    return ST_ERRNO_SUCCESS;
}

ST_ERR ThresholdLevelStrategy::writeImpl(const std::string& filename, const void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    json11::Json::object json_obj = json.object_items();
    const s_msg_algo_threshold_level* level = static_cast<const s_msg_algo_threshold_level*>(data);
    json_obj["threshold_level"] =
        json11::Json::object{ { "verify_threshold_level", level->verify_threshold_level }, { "liveness_threshold_level", level->liveness_threshold_level } };

    return koks_db_write_file(filename, json11::Json(json_obj).dump(), buffer.str());
}

// ThresholdStrategy 实现
ST_ERR ThresholdStrategy::readImpl(const std::string& filename, void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    s_msg_algo_threshold* threshold = static_cast<s_msg_algo_threshold*>(data);
    threshold->verify_threshold = json["threshold"]["verify_threshold"].number_value();
    threshold->liveness_threshold = json["threshold"]["liveness_threshold"].number_value();

    return ST_ERRNO_SUCCESS;
}

ST_ERR ThresholdStrategy::writeImpl(const std::string& filename, const void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    json11::Json::object json_obj = json.object_items();
    const s_msg_algo_threshold* threshold = static_cast<const s_msg_algo_threshold*>(data);
    json_obj["threshold"] = json11::Json::object{ { "verify_threshold", threshold->verify_threshold }, { "liveness_threshold", threshold->liveness_threshold } };

    return koks_db_write_file(filename, json11::Json(json_obj).dump(), buffer.str());
}

// KeyStrategy 实现
ST_ERR KeyStrategy::readImpl(const std::string& filename, void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    std::vector<uint8_t> key;
    auto value = json["key"]["value"];
    //统计value array数量
    if (!value.is_array() || value.array_items().size() != 16) {
        LOG_WARNING("key value error\n");
        return ST_ERRNO_FAIL;
    }
    for (auto& iter : value.array_items()) {
        if (!iter.is_number()) {
            LOG_ERROR("key value error\n");
            return ST_ERRNO_FAIL;
        }
        if (iter.int_value() < 0 || iter.int_value() > 31) {
            LOG_ERROR("key value error\n");
            return ST_ERRNO_FAIL;
        }
        key.push_back(iter.int_value());
    }
    if (key.size() != 16) {
        LOG_ERROR("key value error\n");
        return ST_ERRNO_FAIL;
    }
    std::vector<uint8_t>* key_ = static_cast<std::vector<uint8_t>*>(data);
    *key_ = key;
    return ST_ERRNO_SUCCESS;
}

ST_ERR KeyStrategy::writeImpl(const std::string& filename, const void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    /*
    "key": {
        "value": [
            0,
            1,
            2
        ]
    }
    */
    //将data 中的16个uint8_t 数据插入到json的key的value array中
    if (!json["key"].is_object() || !json["key"]["value"].is_array()) {
        LOG_ERROR("key value error\n");
        return ST_ERRNO_FAIL;
    }
    json11::Json::array key;
    const uint8_t* key_data = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < 16; ++i) {
        key.push_back(key_data[i]);
    }
    json11::Json::object json_obj = json.object_items();
    json_obj["key"] = json11::Json::object{ { "value", key } };

    return koks_db_write_file(filename, json11::Json(json_obj).dump(), buffer.str());
}

// AimiscStrategy 实现
ST_ERR AimiscStrategy::readImpl(const std::string& filename, void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    if (!json["misc"].is_object()) {
        json11::Json::object misc_obj = { { "mustachioed", false }, { "device_type", json11::Json::object{ { "type", 0 } } } };
        json11::Json::object json_obj = json.object_items();
        json_obj["misc"] = misc_obj;
        json = json11::Json(json_obj);
        std::ofstream outfile(filename);
        if (!outfile.is_open()) {
            LOG_ERROR("%s open failed!\n", filename.c_str());
            return ST_ERRNO_FAIL;
        }

        outfile << json11::Json(json_obj).dump();
        outfile.close();
    }
    s_msg_misc_ai_config* misc = static_cast<s_msg_misc_ai_config*>(data);
    misc->beard_enable = json["misc"]["mustachioed"].bool_value();
    misc->device_type = json["misc"]["device_type"]["type"].int_value();
    return ST_ERRNO_SUCCESS;
}

ST_ERR AimiscStrategy::writeImpl(const std::string& filename, const void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    json11::Json::object json_obj = json.object_items();
    const s_msg_misc_ai_config* mustachioed = static_cast<const s_msg_misc_ai_config*>(data);
    json_obj["misc"] = json11::Json::object{ { "mustachioed", mustachioed->beard_enable }, { "device_type", json11::Json::object{ { "type", mustachioed->device_type } } } };

    return koks_db_write_file(filename, json11::Json(json_obj).dump(), buffer.str());
}

ST_ERR BlurStrategy::readImpl(const std::string& filename, void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    s_msg_blur_config* blur = static_cast<s_msg_blur_config*>(data);
    blur->enable = json["blur"]["enable"].bool_value();
    blur->blur_threshold = json["blur"]["threshold"].number_value();

    return ST_ERRNO_SUCCESS;
}

ST_ERR BlurStrategy::writeImpl(const std::string& filename, const void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    json11::Json::object json_obj = json.object_items();
    const s_msg_blur_config* threshold = static_cast<const s_msg_blur_config*>(data);
    json_obj["blur"] = json11::Json::object{ { "enable", threshold->enable }, { "threshold", threshold->blur_threshold } };

    return koks_db_write_file(filename, json11::Json(json_obj).dump(), buffer.str());
}
#ifdef KOKS_FEATURE_PALM
ST_ERR PalmStrategy::readImpl(const std::string& filename, void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    struct PalmModuleLevel* lvl = (struct PalmModuleLevel*)data;
    //json11::Json& palm = json["palm"];
    if (!json["palm"].is_null()) {
        lvl->detection_levle = (enum PalmConfidenceLevel)json["palm"]["detection_levle"].int_value();
        lvl->brightness_levle = (enum PalmConfidenceLevel)json["palm"]["brightness_levle"].int_value();
        lvl->match_levle = (enum PalmConfidenceLevel)json["palm"]["match_levle"].int_value();
        lvl->liveness_levle = (enum PalmConfidenceLevel)json["palm"]["liveness_levle"].int_value();
    }
    LOGI("Palm level: detection=%d, brightness=%d, match=%d, liveness=%d", lvl->detection_levle, lvl->brightness_levle, lvl->match_levle, lvl->liveness_levle);
    return ST_ERRNO_SUCCESS;
}

ST_ERR PalmStrategy::writeImpl(const std::string& filename, const void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    json11::Json::object json_obj = json.object_items();
    struct PalmModuleLevel* lvl = (struct PalmModuleLevel*)data;
    json_obj["palm"] = json11::Json::object{
        { "detection_levle", lvl->detection_levle },
        { "brightness_levle", lvl->brightness_levle},
        { "match_levle", lvl->match_levle },
        { "liveness_levle", lvl->liveness_levle},
    };

    return koks_db_write_file(filename, json11::Json(json_obj).dump(), buffer.str());
}
#endif

#ifdef KOKS_FEATURE_LCD_EMOTION
ST_ERR LcdEmotionStrategy::readImpl(const std::string& filename, void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    uint8_t* EmotionDefaultIndex = (uint8_t*)data;
    if (!json["lcd_emotion"].is_null()) {
        *EmotionDefaultIndex = json["lcd_emotion"]["emotion_default_index"].int_value();
    }else {
        LOG_WARNING("lcd_emotion is null, use default value 1\n");
        *EmotionDefaultIndex = 1;
    }

    LOG_INFO("Read LcdEmotionDefaultIndex: %d\n", *EmotionDefaultIndex);
    return ST_ERRNO_SUCCESS;
}

ST_ERR LcdEmotionStrategy::writeImpl(const std::string& filename, const void* data)
{
    std::stringstream buffer;
    json11::Json json;
    if (koks_db_read_file(filename, buffer, json) != ST_ERRNO_SUCCESS) {
        return ST_ERRNO_FAIL;
    }

    json11::Json::object json_obj = json.object_items();

    uint8_t* EmotionDefaultIndex = (uint8_t*)data;


    json_obj["lcd_emotion"] = json11::Json::object{
        { "emotion_default_index", *EmotionDefaultIndex },
    };

    return koks_db_write_file(filename, json11::Json(json_obj).dump(), buffer.str());
}

#endif


} // namespace sense_koks

} // namespace st

