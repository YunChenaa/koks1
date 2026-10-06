#include "koks_project_def.h"

#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <imp_audio.h>
#include <imp_dmic.h>
#include <imp_log.h>
#include <imp_common.h>
#include <imp_utils.h>

#define TAG "audio_test"

#define AI_BASIC_TEST_RECORD_FILE "/tmp/audio_test.pcm"
#define AI_BASIC_TEST_RECORD_NUM 50

#define SAMPLE_RATE KOKS_FEATURE_AUDIO_SAMPLE_RATE
#define AO_TEST_SAMPLE_TIME 20
#define AO_TEST_BUF_SIZE (SAMPLE_RATE * sizeof(short) * AO_TEST_SAMPLE_TIME / 1000)

// 1000 Hz
#define FREQUENCY 1000.0
// 0.1 秒
#define ANALYSIS_WINDOW 0.1
#define MIN_DETECTED_WINDOWS 10
// 2 秒
#define DURATION 2

#define AMPLITUDE 0.8

static void *_ai_basic_record_test_thread(void *argv);
static void *_ao_test_play_thread(void *argv);

// 生产特定频率的声音
static int generate_audio(unsigned char *buf, int buf_size)
{
    static int index = 0;
    int samples = buf_size / sizeof(int16_t);
    int16_t *p = (int16_t *)(buf);

    for (int i = 0; i < samples; i++)
    {
        double sample = AMPLITUDE * sin(2 * M_PI * FREQUENCY * (index + i) / SAMPLE_RATE);
        p[i] = (int16_t)(sample * 32767.0);
    }

    index += samples;
    return buf_size;
}

// 检测特定频率
static float detect_frequency(const int16_t *samples, int num_samples, float target_freq)
{
    const float k = (int)(0.5 + (num_samples * target_freq) / SAMPLE_RATE);
    const float omega = (2.0 * M_PI * k) / num_samples;
    const float coeff = 2.0 * cos(omega);

    float q0 = 0.0f;
    float q1 = 0.0f;
    float q2 = 0.0f;

    for (int i = 0; i < num_samples; i++)
    {
        float sample = samples[i] / 32768.0f;

        q0 = coeff * q1 - q2 + sample;
        q2 = q1;
        q1 = q0;
    }

    // 计算能量
    float energy = q1 * q1 + q2 * q2 - coeff * q1 * q2;
    return energy;
}

static int check_result(float threshold)
{
    // 打开录音文件
    FILE *file = fopen(AI_BASIC_TEST_RECORD_FILE, "rb");
    if (!file)
    {
        perror("无法打开录音文件");
        return -1;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 计算样本数
    int total_samples = file_size / sizeof(int16_t);
    int window_samples = SAMPLE_RATE * ANALYSIS_WINDOW;

    // 分配缓冲区
    int16_t *buffer = (int16_t *)malloc(file_size);
    if (!buffer)
    {
        perror("内存分配失败");
        fclose(file);
        return -1;
    }

    // 读取整个文件
    if (fread(buffer, sizeof(int16_t), total_samples, file) != total_samples)
    {
        perror("读取文件失败");
        free(buffer);
        fclose(file);
        return -1;
    }
    fclose(file);

    // 分析音频
    int windows = total_samples / window_samples;
    int detected_windows = 0;

    for (int i = 0; i < windows; i++)
    {
        const int16_t *window_start = buffer + i * window_samples;
        float energy = detect_frequency(window_start, window_samples, FREQUENCY);

        // 归一化能量
        float normalized_energy = energy / (window_samples * window_samples);
        printf("  %d: energy %.3f, ok %d\n", i, normalized_energy, (normalized_energy >= threshold));
        if (normalized_energy >= threshold)
        {
            detected_windows++;
        }
    }

    // 结果判断
    printf("windows %d, detected_windows %d, threshold %.3f\n", windows, detected_windows, threshold);
    free(buffer);
    return (detected_windows >= MIN_DETECTED_WINDOWS) ? 0 : -2;
}

static void *_ai_basic_record_test_thread(void *argv)
{
    int ret = -1;
    int record_num = 0;

    FILE *record_file = fopen(AI_BASIC_TEST_RECORD_FILE, "wb");
    if (record_file == NULL)
    {
        IMP_LOG_ERR(TAG, "fopen %s failed\n", AI_BASIC_TEST_RECORD_FILE);
        return NULL;
    }

    /* Step.2 set public attribute of AI device */
    int devID = 1;
    IMPAudioIOAttr attr;
    attr.samplerate = (IMPAudioSampleRate)SAMPLE_RATE;
    attr.bitwidth = AUDIO_BIT_WIDTH_16;
    attr.soundmode = AUDIO_SOUND_MODE_MONO;
    attr.frmNum = 40;
    attr.numPerFrm = SAMPLE_RATE * 40 / 1000;
    attr.chnCnt = 1;
    ret = IMP_AI_SetPubAttr(devID, &attr);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "set ai %d attr err: %d\n", devID, ret);
        return NULL;
    }
    memset(&attr, 0x0, sizeof(attr));
    ret = IMP_AI_GetPubAttr(devID, &attr);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "get ai %d attr err: %d\n", devID, ret);
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr samplerate : %d\n", attr.samplerate);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr   bitwidth : %d\n", attr.bitwidth);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr  soundmode : %d\n", attr.soundmode);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr     frmNum : %d\n", attr.frmNum);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr  numPerFrm : %d\n", attr.numPerFrm);
    IMP_LOG_INFO(TAG, "Audio In GetPubAttr     chnCnt : %d\n", attr.chnCnt);

    /* Step.3 enable AI device */
    ret = IMP_AI_Enable(devID);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "enable ai %d err\n", devID);
        return NULL;
    }

    /* Step.4  set audio channel attribute of AI device */
    int chnID = 0;
    IMPAudioIChnParam chnParam;
    chnParam.usrFrmDepth = 40;
    chnParam.aecChn = AUDIO_AEC_CHANNEL_FIRST_LEFT;
    ret = IMP_AI_SetChnParam(devID, chnID, &chnParam);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "set ai %d channel %d attr err: %d\n", devID, chnID, ret);
        return NULL;
    }
    memset(&chnParam, 0x0, sizeof(chnParam));
    ret = IMP_AI_GetChnParam(devID, chnID, &chnParam);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "get ai %d channel %d attr err: %d\n", devID, chnID, ret);
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio In GetChnParam usrFrmDepth : %d\n", chnParam.usrFrmDepth);

    /* Step 4: enable AI channel. */
    ret = IMP_AI_EnableChn(devID, chnID);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio Record enable channel failed\n");
        return NULL;
    }

    /* Step.5 Set audio channel volume */
    int chnVol = 60;
    ret = IMP_AI_SetVol(devID, chnID, chnVol);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio Record set volume failed\n");
        return NULL;
    }
    ret = IMP_AI_GetVol(devID, chnID, &chnVol);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio Record get volume failed\n");
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio In GetVol    vol : %d\n", chnVol);

    /* Step.6 Set audio channel gain */
    int aigain = 23;
    ret = IMP_AI_SetGain(devID, chnID, aigain);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio Record Set Gain failed\n");
        return NULL;
    }
    ret = IMP_AI_GetGain(devID, chnID, &aigain);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio Record Get Gain failed\n");
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio In GetGain    gain : %d\n", aigain);

    /* Step.7  enable AI algorithm */
    // ret = IMP_AI_EnableAlgo(devID, chnID);
    // if(ret != 0) {
    // 	IMP_LOG_ERR(TAG, "IMP_AI_EnableAlgo failed\n");
    // 	return NULL;
    // }

    while (1)
    {
        /* Step.8  get audio record frame */
        ret = IMP_AI_PollingFrame(devID, chnID, 1000);
        if (ret != 0)
        {
            IMP_LOG_ERR(TAG, "Audio Polling Frame Data error\n");
        }
        IMPAudioFrame frm;
        ret = IMP_AI_GetFrame(devID, chnID, &frm, BLOCK);
        if (ret != 0)
        {
            IMP_LOG_ERR(TAG, "Audio Get Frame Data error\n");
            return NULL;
        }

        /* Step.9 Save the recording data to a file */
        fwrite(frm.virAddr, 1, frm.len, record_file);

        /* Step.10  release the audio record frame */
        ret = IMP_AI_ReleaseFrame(devID, chnID, &frm);
        if (ret != 0)
        {
            IMP_LOG_ERR(TAG, "Audio release frame data error\n");
            return NULL;
        }

        if (++record_num >= AI_BASIC_TEST_RECORD_NUM)
            break;
    }

    // sleep(3);
    /* Step.11 disable AI algorithm */
    // ret = IMP_AI_DisableAlgo(devID, chnID);
    // if(ret != 0) {
    // 	IMP_LOG_ERR(TAG, "IMP_AI_DisableAlgo failed\n");
    // 	return NULL;
    // }

    /* Step.12 disable the audio channel */
    ret = IMP_AI_DisableChn(devID, chnID);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio channel disable error\n");
        return NULL;
    }

    /* Step.13 disable the audio devices */
    ret = IMP_AI_Disable(devID);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio device disable error\n");
        return NULL;
    }

    printf("Audio record finished\n");
    fclose(record_file);
    pthread_exit(0);
}

static void *_ao_test_play_thread(void *argv)
{
    unsigned char *buf = NULL;
    int size = 0;
    int ret = -1;

    buf = (unsigned char *)malloc(AO_TEST_BUF_SIZE);
    if (buf == NULL)
    {
        IMP_LOG_ERR(TAG, "[ERROR] %s: malloc audio buf error\n", __func__);
        return NULL;
    }

    /* Step.1 set public attribute of AO device */
    int devID = 0;
    IMPAudioIOAttr attr;
    attr.samplerate = (IMPAudioSampleRate)SAMPLE_RATE;
    attr.bitwidth = AUDIO_BIT_WIDTH_16;
    attr.soundmode = AUDIO_SOUND_MODE_MONO;
    attr.frmNum = 20;
    attr.numPerFrm = SAMPLE_RATE * 40 / 1000;
    attr.chnCnt = 1;
    ret = IMP_AO_SetPubAttr(devID, &attr);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "set ao %d attr err: %d\n", devID, ret);
        return NULL;
    }
    memset(&attr, 0x0, sizeof(attr));
    ret = IMP_AO_GetPubAttr(devID, &attr);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "get ao %d attr err: %d\n", devID, ret);
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio Out GetPubAttr samplerate:%d\n", attr.samplerate);
    IMP_LOG_INFO(TAG, "Audio Out GetPubAttr   bitwidth:%d\n", attr.bitwidth);
    IMP_LOG_INFO(TAG, "Audio Out GetPubAttr  soundmode:%d\n", attr.soundmode);
    IMP_LOG_INFO(TAG, "Audio Out GetPubAttr     frmNum:%d\n", attr.frmNum);
    IMP_LOG_INFO(TAG, "Audio Out GetPubAttr  numPerFrm:%d\n", attr.numPerFrm);
    IMP_LOG_INFO(TAG, "Audio Out GetPubAttr     chnCnt:%d\n", attr.chnCnt);

    /* Step.2 enable AO device */
    ret = IMP_AO_Enable(devID);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "enable ao %d err\n", devID);
        return NULL;
    }

    /* Step.3 enable AO channel */
    int chnID = 0;
    ret = IMP_AO_EnableChn(devID, chnID);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio play enable channel failed\n");
        return NULL;
    }

    /* Step.4 Set audio channel volume */
    int chnVol = 50;
    ret = IMP_AO_SetVol(devID, chnID, chnVol);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio Play set volume failed\n");
        return NULL;
    }
    ret = IMP_AO_GetVol(devID, chnID, &chnVol);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio Play get volume failed\n");
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio Out GetVol    vol:%d\n", chnVol);

    /* Step.5 Set audio channel gain */
    int aogain = 28;
    ret = IMP_AO_SetGain(devID, chnID, aogain);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio Play Set Gain failed\n");
        return NULL;
    }
    ret = IMP_AO_GetGain(devID, chnID, &aogain);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio Play Get Gain failed\n");
        return NULL;
    }
    IMP_LOG_INFO(TAG, "Audio Out GetGain    gain : %d\n", aogain);

    /* Step.6 enable AO algorithm */
    // ret = IMP_AO_EnableAlgo(devID, chnID);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "IMP_AO_EnableAlgo failed\n");
        return NULL;
    }

    const int max_size = SAMPLE_RATE * DURATION * sizeof(int16_t);
    int total_size = 0;
    while (total_size < max_size)
    {
        size = generate_audio(buf, AO_TEST_BUF_SIZE);
        total_size += size;

        /* Step.7 send frame data */
        IMPAudioFrame frm;
        frm.virAddr = (uint32_t *)buf;
        frm.len = size;
        ret = IMP_AO_SendFrame(devID, chnID, &frm, BLOCK);
        if (ret != 0)
        {
            IMP_LOG_ERR(TAG, "send Frame Data error!\n");
            return NULL;
        }

        /* Step.8 view playback frame status */
        IMPAudioOChnState play_status;
        ret = IMP_AO_QueryChnStat(devID, chnID, &play_status);
        if (ret != 0)
        {
            IMP_LOG_ERR(TAG, "IMP_AO_QueryChnStat error!\n");
            return NULL;
        }

        IMP_LOG_INFO(TAG, "Play: TotalNum %d, FreeNum %d, BusyNum %d\n",
                     play_status.chnTotalNum, play_status.chnFreeNum, play_status.chnBusyNum);
    }

    /* Step.9 waiting for the last audio data to finish playing */
    ret = IMP_AO_FlushChnBuf(devID, chnID);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "IMP_AO_FlushChnBuf error\n");
        return NULL;
    }

    /* Step.10 disable AO algorithm */
    // ret = IMP_AO_DisableAlgo(devID, chnID);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "IMP_AO_DisableAlgo error\n");
        return NULL;
    }

    /* Step.11 disable the audio channel */
    ret = IMP_AO_DisableChn(devID, chnID);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio channel disable error\n");
        return NULL;
    }

    /* Step.12 disable the audio devices */
    ret = IMP_AO_Disable(devID);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "Audio device disable error\n");
        return NULL;
    }

    printf("Audio play finished\n");
    free(buf);
    pthread_exit(0);
}

int audio_test(float threshold)
{
    int ret = -1;
    pthread_t record_thread_id;
    pthread_t play_thread_id;

    printf("audio test start, threshold %.3f\n", threshold);
    /* Start audio recording thread */
    ret = pthread_create(&record_thread_id, NULL, _ai_basic_record_test_thread, NULL);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "[ERROR] %s: pthread_create Audio Record failed\n", __func__);
        return -1;
    }

    /* Create audio playback thread */
    ret = pthread_create(&play_thread_id, NULL, _ao_test_play_thread, NULL);
    if (ret != 0)
    {
        IMP_LOG_ERR(TAG, "[ERROR] %s: pthread_create Audio play failed\n", __func__);
        return -1;
    }

    pthread_join(play_thread_id, NULL);
    pthread_join(record_thread_id, NULL);
    return check_result(threshold);
}
