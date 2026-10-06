/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-16 17:09:14
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-29 14:34:21
 * @FilePath: /sense-koks/application/media_core/src/st_koks_mediamanager.cpp
 */
#include "st_koks_mediamanager.h"
#include "os_utils.h"
#include "st_koks_isp.h"
#include "sys_logger.h"
#include "koks1_config.h"
namespace st
{
namespace sense_koks
{
/**
 * @name: MediaManager
 * @brief: 媒体管理类构造函数
 * @return {*}
 */
MediaManager::MediaManager(void)
{
    const auto num_fs = koks_config.media.video.frame_source.size();
    vector_framesource_.reserve(num_fs);
    for (size_t i = 0; i < num_fs; ++i) {
        vector_framesource_.emplace_back(i);
    }

    const auto num_venc = koks_config.media.video.venc.size();
    vector_venc_.reserve(num_venc);
    for (size_t i = 0; i < num_venc; ++i) {
        vector_venc_.emplace_back(i);
    }

#ifdef KOKS_FEATURE_VIDEO
    vector_osd_.reserve(1);
    vector_osd_.emplace_back(DEV_OSD_0);

    vector_audio_input_.reserve(1);
    vector_audio_input_.emplace_back(DEV_AUDIO_INPUT_0);

    vector_audio_output_.reserve(1);
    vector_audio_output_.emplace_back(DEV_AUDIO_OUTPUT_0);
#ifdef AUDIO_CODEC
    vector_audio_encode_.reserve(1);
    vector_audio_encode_.emplace_back(DEV_AUDIO_ENCODE_0);

    vector_audio_decode_.reserve(1);
    vector_audio_decode_.emplace_back(DEV_AUDIO_DECODE_0);
#endif
#endif //KOKS_FEATURE_VIDEO

    InitVideo();
}

/**
 * @name: ~MediaManager
 * @brief: 媒体管理类析构函数
 * @return {*}
 */
MediaManager::~MediaManager(void)
{
    LOG_INFO("~MediaManager\n");
    // Stop();
    // DeInit();
}

/**
 * @name: GetInstance
 * @brief: 返回设备管理类的单例实例
 * @param {void}
 * @return {MediaManager &}
 */
MediaManager& MediaManager::GetInstance(void)
{
    static MediaManager dev_manager;
    return dev_manager;
}

ST_ERR MediaManager::InitVideo()
{
    LOG_INFO("InitVideo start\n");
    auto ret = Init(koks_config.media, MediaType::VIDEO);
    if (ret)
        LOG_ERROR("media_manager.Init failed with %d\n", ret);
    ret = Start(MediaType::VIDEO);
    if (ret)
        LOG_ERROR("media_manager.Start failed with %d\n", ret);

    LOG_INFO("InitVideo done\n");
}

/**
 * @name: Init
 * @brief: 初始化管理类
 * @param {const json11::Json&} config_json 配置文件
 * @return {ST_ERR}
 */
ST_ERR MediaManager::Init(const struct MediaConfig& media_config, MediaType type)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    LOG_DEBUG("Init\n");
    if (type == MediaType::VIDEO) {
        ret = Isp::GetInstance().Init(media_config.video.isp);
        MISC_CHK_STATUS_REPORT(ret, ret, "Isp::GetInstance().Init with %d\n", ret);

        for (auto& iter : media_config.video.frame_source) {
            if (vector_framesource_[iter.dev].Init(iter) != ST_ERRNO_SUCCESS) {
                LOG_ERROR("vector_framesource_ dev %d init failed!\n", iter.dev);
                return ST_ERRNO_FAIL;
            }
            LOG_DEBUG("vector_framesource_ dev %d init success!\n", iter.dev);
        }

        for (auto& iter : media_config.video.venc) {
            if (vector_venc_[iter.dev].Init(iter) != ST_ERRNO_SUCCESS) {
                LOG_ERROR("vector_venc_ dev %d init failed!\n", iter.dev);
                return ST_ERRNO_FAIL;
            }
            LOG_DEBUG("vector_venc_ dev %d init success!\n", iter.dev);
        }

        // auto VencChn0_Cb = [&](uint32_t chn, SpEncodeVideoFrameInfo& frame) -> ST_ERR {
        //     LOG_VERBOSE("pack count %ld, seq %ld\n", frame->packCount, frame->seq);
        //     return ST_ERRNO_SUCCESS;
        // };
        // vector_venc_[DEV_VENC_0].SetFrameProcCb(rgb_h265_channle, VencChn0_Cb);
    }
#ifdef KOKS_FEATURE_AUDIO
	else if (type == MediaType::AUDIO) {

        for (auto& iter : media_config.audio.input) {
            if (vector_audio_input_[iter.dev].Init(iter) != ST_ERRNO_SUCCESS) {
                LOG_ERROR("vector_audio_input_ dev %d init failed!\n", iter.dev);
                return ST_ERRNO_FAIL;
            }
            LOG_DEBUG("vector_audio_input_ dev %d init success!\n", iter.dev);
        }

        for (auto& iter : media_config.audio.output) {
            if (vector_audio_output_[iter.dev].Init(iter) != ST_ERRNO_SUCCESS) {
                LOG_ERROR("vector_audio_output_ dev %d init failed!\n", iter.dev);
                return ST_ERRNO_FAIL;
            }
            LOG_DEBUG("vector_audio_output_ dev %d init success!\n", iter.dev);
        }
#ifdef AUDIO_CODEC
        for (auto& iter : config_json["audio"]["encode"].array_items()) {
            if (iter["dev"]["enable"].bool_value() == true) {
                if (vector_audio_encode_[iter.dev].Init(iter) != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("vector_audio_encode_ dev %d init failed!\n", iter.dev);
                    return ST_ERRNO_FAIL;
                }
                LOG_DEBUG("vector_audio_encode_ dev %d init success!\n", iter.dev);
            }
        }

        for (auto& iter : config_json["audio"]["decode"].array_items()) {
            if (vector_audio_decode_[iter.dev].Init(iter) != ST_ERRNO_SUCCESS) {
                LOG_ERROR("vector_audio_decode_ dev %d init failed!\n", iter.dev);
                return ST_ERRNO_FAIL;
            }
            LOG_DEBUG("vector_audio_decode_ dev %d init success!\n", iter.dev);
        }
        vector_audio_encode_[DEV_AUDIO_ENCODE_0].SetFrameProcCb([&](SpCodecsAudioStreamInfo& stream) -> ST_ERR {
            LOG_DEBUG("audio encode frame len %d,seq %d,ts %lld\n", stream->len, stream->seq, stream->timeStamp);
            return ST_ERRNO_SUCCESS;
        });

#if 0
    vector_audio_decode_[DEV_AUDIO_DECODE_0].SetFrameProcCb([&](void) -> SpCodecsAudioStreamInfo {
        // char buf_g711[200];
        static uint32_t i = 0;
        IMPAudioStream* _stream = new IMPAudioStream;
        // _stream->seq = i++;
        // _stream->timeStamp = GetCurTimeUs();
        // _stream->len = 200;
        // _stream->stream = (uint8_t*)buf_g711;
        // _stream->phyAddr = 0;
        auto* audio_input = MediaManager::GetInstance().GetDeviceAudioInput(DEV_AUDIO_INPUT_0);
        SpInputAudioFrameInfo sp_in_frame(new IMPAudioFrame, [=](IMPAudioFrame* stream) -> void {
            ST_ERR ret = audio_input->ReleaseFrame(*stream);
            delete stream;
            MISC_CHK_STATUS_REPORT_NO_RET(ret, "ReleaseFrame failed with %#x\n", ret);
            LOG_VERBOSE("ReleaseStream success!\n");
        });

        int ret = 0;
        ret = audio_input->GetFrame(*sp_in_frame);
        MISC_CHK_STATUS_REPORT_EXEC(ret, nullptr, delete _stream, "GetFrame failed with %#x\n", ret);

        ret = vector_audio_encode_[DEV_AUDIO_ENCODE_0].SendGetStream(*sp_in_frame, *_stream);
        MISC_CHK_STATUS_REPORT_EXEC(ret, nullptr, delete _stream, "SendGetStream failed with %#x\n", ret);
        SpCodecsAudioStreamInfo sp_enc_stream(_stream, [=](IMPAudioStream* stream) -> void {
            ST_ERR ret = ST_ERRNO_SUCCESS;
            LOG_VERBOSE("ReleaseStream seq %d success!\n", stream->seq);
            vector_audio_encode_[DEV_AUDIO_ENCODE_0].ReleaseStream(*stream);
            delete stream;
        });
        return sp_enc_stream;
    });
#endif
#endif
    }
#endif
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: DeInit
 * @brief: 反初始化管理类
 * @param {void}
 * @return {ST_ERR}
 */
ST_ERR MediaManager::DeInit(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    const auto num_venc = koks_config.media.video.venc.size();
    for (size_t i = 0; i < num_venc; ++i) {
        ret = vector_venc_[i].DeInit();
        // MISC_CHK_STATUS_REPORT(ret, ret, "vector_venc_[%d] DeInit with %d\n", i, ret);
    }

    const auto num_fs = koks_config.media.video.frame_source.size();
    for (size_t i = 0; i < num_fs; ++i) {
        ret = vector_framesource_[i].DeInit();
        // MISC_CHK_STATUS_REPORT(ret, ret, "vector_framesource_[%d] DeInit with %d\n", i, ret);
    }

#ifdef KOKS_FEATURE_VIDEO
    ret = vector_osd_[DEV_OSD_0].DeInit();
    // MISC_CHK_STATUS_REPORT(ret, ret, "vector_osd_[%d] DeInit with %d\n", DEV_OSD_0, ret);
#endif

    ret = Isp::GetInstance().Deinit();
    // MISC_CHK_STATUS_REPORT(ret, ret, "Isp::GetInstance().Deinit with %d\n", ret);

#ifdef KOKS_FEATURE_AUDIO
    ret = vector_audio_input_[DEV_AUDIO_INPUT_0].DeInit();
    // MISC_CHK_STATUS_REPORT(ret, ret, "vector_audio_input_[%d] DeInit with %d\n", DEV_AUDIO_INPUT_0, ret);

    ret = vector_audio_output_[DEV_AUDIO_OUTPUT_0].DeInit();
    // MISC_CHK_STATUS_REPORT(ret, ret, "vector_audio_output_[%d] DeInit with %d\n", DEV_AUDIO_OUTPUT_0, ret);
#ifdef AUDIO_CODEC
    ret = vector_audio_encode_[DEV_AUDIO_ENCODE_0].DeInit();
    // MISC_CHK_STATUS_REPORT(ret, ret, "vector_audio_encode_[%d] DeInit with %d\n", DEV_AUDIO_ENCODE_0, ret);

    ret = vector_audio_decode_[DEV_AUDIO_DECODE_0].DeInit();
    // MISC_CHK_STATUS_REPORT(ret, ret, "vector_audio_decode_[%d] DeInit with %d\n", DEV_AUDIO_DECODE_0, ret);
#endif
#endif // KOKS_FEATURE_AUDIO

    LOG_DEBUG("DeInit\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Start
 * @brief: 启动管理类
 * @param {void}
 * @return {ST_ERR}
 */
ST_ERR MediaManager::Start(MediaType type)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (type == MediaType::VIDEO) {

        // ret = vector_framesource_[DEV_FRAME_SOURCE_2].Bind(vector_framesource_[DEV_FRAME_SOURCE_2].GetIMPCell(), vector_venc_[DEV_VENC_0].GetIMPCell());
        // MISC_CHK_STATUS_REPORT(ret, ret, "framesource %d Bind enc group %d failed with %d\n", vector_framesource_[DEV_FRAME_SOURCE_2].GetSourceID(),
        //                        vector_venc_[DEV_VENC_0].GetGroup(), ret);

        // ret = vector_framesource_[DEV_FRAME_SOURCE_3].Bind(vector_framesource_[DEV_FRAME_SOURCE_3].GetIMPCell(), vector_venc_[DEV_VENC_1].GetIMPCell());
        // MISC_CHK_STATUS_REPORT(ret, ret, "framesource %d Bind enc group %d failed with %d\n", vector_framesource_[DEV_FRAME_SOURCE_3].GetSourceID(),
        //                        vector_venc_[DEV_VENC_1].GetGroup(), ret);

        ret = vector_framesource_[DEV_FRAME_SOURCE_0].Start();
        MISC_CHK_STATUS_REPORT(ret, ret, "vector_framesource_[%d] Start with %d\n", DEV_FRAME_SOURCE_0, ret);
        ret = vector_framesource_[DEV_FRAME_SOURCE_1].Start();
        MISC_CHK_STATUS_REPORT(ret, ret, "vector_framesource_[%d] Start with %d\n", DEV_FRAME_SOURCE_1, ret);
        // ret = vector_framesource_[DEV_FRAME_SOURCE_2].Start();
        // MISC_CHK_STATUS_REPORT(ret, ret, "vector_framesource_[%d] Start with %d\n", DEV_FRAME_SOURCE_2, ret);
        // ret = vector_framesource_[DEV_FRAME_SOURCE_3].Start();
        // MISC_CHK_STATUS_REPORT(ret, ret, "vector_framesource_[%d] Start with %d\n", DEV_FRAME_SOURCE_3, ret);

        // ret = vector_venc_[DEV_VENC_0].Start();
        // MISC_CHK_STATUS_REPORT(ret, ret, "vector_venc_[%d] Start with %d\n", DEV_VENC_0, ret);
        // ret = vector_venc_[DEV_VENC_1].Start();
        // MISC_CHK_STATUS_REPORT(ret, ret, "vector_venc_[%d] Start with %d\n", DEV_VENC_1, ret);
    }
#ifdef KOKS_FEATURE_AUDIO
	else if (type == MediaType::AUDIO) {
        ret = vector_audio_input_[DEV_AUDIO_INPUT_0].Start();
        MISC_CHK_STATUS_REPORT(ret, ret, "vector_audio_input_[%d] Start with %d\n", DEV_AUDIO_INPUT_0, ret);

        ret = vector_audio_output_[DEV_AUDIO_OUTPUT_0].Start();
        MISC_CHK_STATUS_REPORT(ret, ret, "vector_audio_output_[%d] Start with %d\n", DEV_AUDIO_OUTPUT_0, ret);
#ifdef AUDIO_CODEC
        // ret = vector_audio_encode_[DEV_AUDIO_ENCODE_0].Start();
        // MISC_CHK_STATUS_REPORT(ret, ret, "vector_audio_encode_[%d] Start with %d\n", DEV_AUDIO_ENCODE_0, ret);

        ret = vector_audio_decode_[DEV_AUDIO_DECODE_0].Start();
        MISC_CHK_STATUS_REPORT(ret, ret, "vector_audio_decode_[%d] Start with %d\n", DEV_AUDIO_DECODE_0, ret);
#endif
    }
#endif //KOKS_FEATURE_AUDIO
    LOG_DEBUG("Start\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Stop
 * @brief: 停止管理类
 * @param {void}
 * @return {ST_ERR}
 */
ST_ERR MediaManager::Stop(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;

    const auto num_venc = koks_config.media.video.venc.size();
    for (size_t i = 0; i < num_venc; ++i) {
        ret = vector_venc_[i].Stop();
        // MISC_CHK_STATUS_REPORT(ret, ret, "vector_venc_[%d] Stop with %d\n", i, ret);
    }

    const auto num_fs = koks_config.media.video.frame_source.size();
    for (size_t i = 0; i < num_fs; ++i) {
        ret = vector_framesource_[i].Stop();
        // MISC_CHK_STATUS_REPORT(ret, ret, "vector_framesource_[%d] Stop with %d\n", i, ret);
    }

#ifdef KOKS_FEATURE_VIDEO
    ret = vector_osd_[DEV_OSD_0].Stop();
    // MISC_CHK_STATUS_REPORT(ret, ret, "vector_osd_[%d] Stop with %d\n", DEV_OSD_0, ret);

    ret = vector_audio_output_[DEV_AUDIO_OUTPUT_0].Stop();
    // MISC_CHK_STATUS_REPORT(ret, ret, "vector_audio_output_[%d] Stop with %d\n", DEV_AUDIO_OUTPUT_0, ret);

    // ret = vector_audio_encode_[DEV_AUDIO_ENCODE_0].Stop();
    // MISC_CHK_STATUS_REPORT(ret, ret, "vector_audio_encode_[%d] Stop with %d\n", DEV_AUDIO_ENCODE_0, ret);

    ret = vector_audio_input_[DEV_AUDIO_INPUT_0].Stop();
    // MISC_CHK_STATUS_REPORT(ret, ret, "vector_audio_input_[%d] Stop with %d\n", DEV_AUDIO_INPUT_0, ret);
#ifdef AUDIO_CODEC
    ret = vector_audio_decode_[DEV_AUDIO_DECODE_0].Stop();
    // MISC_CHK_STATUS_REPORT(ret, ret, "vector_audio_decode_[%d] Stop with %d\n", DEV_AUDIO_DECODE_0, ret);
#endif
#endif //KOKS_FEATURE_VIDEO
    LOG_DEBUG("Stop\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: GetDeviceFrameSource
 * @brief: 获取视频源设备
 * @param {uint16_t} idx 设备ID
 * @return {DeviceFrameSource*} 视频源设备指针
 */
DeviceFrameSource* MediaManager::GetDeviceFrameSource(uint16_t idx)
{
    MISC_CHK_CONDITION_REPORT(idx < vector_framesource_.size(), nullptr, "err FrameSource idx %d >= max %d\n", vector_framesource_.size());
    return &vector_framesource_[idx];
}

/**
 * @name: GetDeviceVenc
 * @brief: 获取视频编码设备
 * @param {uint16_t} idx 设备ID
 * @return {DeviceVenc*} 视频编码设备指针
 */
DeviceVenc* MediaManager::GetDeviceVenc(uint16_t idx)
{
    MISC_CHK_CONDITION_REPORT(idx < vector_venc_.size(), nullptr, "err venc idx %d >= max %d\n", vector_venc_.size());
    return &vector_venc_[idx];
}

#ifdef KOKS_FEATURE_VIDEO
/**
 * @name: GetDeviceOSD
 * @brief: 获取OSD设备
 * @param {uint16_t} idx 设备ID
 * @return {DeviceOSD*} OSD设备指针
 */
DeviceOSD* MediaManager::GetDeviceOSD(uint16_t idx)
{
    MISC_CHK_CONDITION_REPORT(idx < vector_osd_.size(), nullptr, "err osd idx %d >= max %d\n", vector_osd_.size());
    return &vector_osd_[idx];
}

/**
 * @name: GetDeviceAudioInput
 * @brief: 获取音频输入设备
 * @param {uint16_t} idx 设备ID
 * @return {DeviceAudioInput*} 音频输入设备指针
 */
DeviceAudioInput* MediaManager::GetDeviceAudioInput(uint16_t idx)
{
    MISC_CHK_CONDITION_REPORT(idx < vector_audio_input_.size(), nullptr, "err audio input idx %d >= max %d\n", vector_audio_input_.size());
    return &vector_audio_input_[idx];
}

/**
 * @name: GetDeviceAudioOutput
 * @brief: 获取音频输出设备
 * @param {uint16_t} idx 设备ID
 * @return {DeviceAudioOutput*} 音频输出设备指针
 */
DeviceAudioOutput* MediaManager::GetDeviceAudioOutput(uint16_t idx)
{
    MISC_CHK_CONDITION_REPORT(idx < vector_audio_output_.size(), nullptr, "err audio output idx %d >= max %d\n", vector_audio_output_.size());
    return &vector_audio_output_[idx];
}
#ifdef AUDIO_CODEC
/**
 * @name: GetDeviceAudioEncode
 * @brief: 获取音频编码设备
 * @param {uint16_t} idx 设备ID
 * @return {DeviceAudioEncode*} 音频编码设备指针
 */
DeviceAudioEncode* MediaManager::GetDeviceAudioEncode(uint16_t idx)
{
    MISC_CHK_CONDITION_REPORT(idx < vector_audio_encode_.size(), nullptr, "err audio encode idx %d >= max %d\n", vector_audio_encode_.size());
    return &vector_audio_encode_[idx];
}

/**
 * @name: GetDeviceAudioDecode
 * @brief: 获取音频解码设备
 * @param {uint16_t} idx 设备ID
 * @return {DeviceAudioDecode*} 音频解码设备指针
 */
DeviceAudioDecode* MediaManager::GetDeviceAudioDecode(uint16_t idx)
{
    MISC_CHK_CONDITION_REPORT(idx < vector_audio_decode_.size(), nullptr, "err audio decode idx %d >= max %d\n", vector_audio_decode_.size());
    return &vector_audio_decode_[idx];
}
#endif
#endif //KOKS_FEATURE_VIDEO
} // namespace sense_koks

} // namespace st