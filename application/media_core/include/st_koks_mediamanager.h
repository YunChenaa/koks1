/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-16 17:09:07
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-25 11:58:28
 * @FilePath: /sense-koks/application/media_core/include/st_koks_mediamanager.h
 */
#pragma once
#include "koks1_config.h"
#include "st_koks_audio.h"
#include "st_koks_errno.h"
#include "st_koks_const.h"
#include <json11.hpp>
#include <st_koks_video.h>
#include <vector>

namespace st
{
namespace sense_koks
{
enum class MediaType {
    VIDEO = 0,
    AUDIO = 1,
};

#define DEV_AUDIO_INPUT_0 0
#define DEV_AUDIO_OUTPUT_0 0
#define DEV_AUDIO_ENCODE_0 0
#define DEV_AUDIO_DECODE_0 0
class MediaManager final
{
    public:
    /**
     * @name: GetInstance
     * @brief:Return the singleton instance of the device manager
     * @param {void}
     * @return {MediaManager &}
     */
    static MediaManager& GetInstance(void);

    /**
     * @name: Init
     * @brief: initialization manager
     * @param {void}
     * @return {ST_ERR}
     */
    ST_ERR Init(const struct MediaConfig& media_config, MediaType type);

    /**
     * @name: Start
     * @brief: Start manager
     * @param {void}
     * @return {ST_ERR}
     */
    ST_ERR Start(MediaType type);

    /**
     * @name:Stop
     * @brief:stop manager
     * @param {void}
     * @return {ST_ERR}
     */
    ST_ERR Stop(void);

    /**
     * @name:DeInit
     * @brief:deinit vpe
     * @param {void}
     * @return {ST_ERR}
     */
    ST_ERR DeInit(void);

    DeviceFrameSource* GetDeviceFrameSource(uint16_t idx);
    DeviceVenc* GetDeviceVenc(uint16_t idx);
#ifdef KOKS_FEATURE_VIDEO
    DeviceOSD* GetDeviceOSD(uint16_t idx);
    DeviceAudioInput* GetDeviceAudioInput(uint16_t idx);
    DeviceAudioOutput* GetDeviceAudioOutput(uint16_t idx);
#ifdef AUDIO_CODEC
    DeviceAudioEncode* GetDeviceAudioEncode(uint16_t idx);
    DeviceAudioDecode* GetDeviceAudioDecode(uint16_t idx);
#endif
#endif
    private:
    /**
     * @name: MediaManager
     * @brief:MediaManager constructor
     * @param {void}
     * @return {}
     */
    MediaManager(void);

    /**
     * @name:~MediaManager
     * @brief:MediaManager destruction
     * @param {*}
     * @return {*}
     */
    ~MediaManager(void);

    /// Disable copy construction
    MediaManager(const MediaManager&) = delete;

    /// Disable assignment construction
    MediaManager& operator=(const MediaManager&) = delete;

    ST_ERR InitVideo();

    private:
    json11::Json config_json;
    std::vector<DeviceVenc> vector_venc_;
    std::vector<DeviceFrameSource> vector_framesource_;
#ifdef KOKS_FEATURE_VIDEO
    std::vector<DeviceOSD> vector_osd_;
    std::vector<DeviceAudioInput> vector_audio_input_;
    std::vector<DeviceAudioOutput> vector_audio_output_;
#ifdef AUDIO_CODEC
    std::vector<DeviceAudioEncode> vector_audio_encode_;
    std::vector<DeviceAudioDecode> vector_audio_decode_;
#endif
#endif
};
} // namespace sense_koks

} // namespace st
