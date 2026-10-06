#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <vector>
#include <cstring>
#include <cstdio>

#include "RTSPAudioServerMediaSubession.h"
#include "st_koks_mediamanager.h"

using namespace st::sense_koks;

// ==================== G.711 A-law 编解码 ====================
static std::queue<std::vector<uint8_t>> g_audioQueue;
static std::mutex g_audioMutex;
static std::condition_variable g_audioCV;

static std::thread g_audioThread;
static std::atomic<bool> g_audioRunning{false};
static std::atomic<bool> g_audioStopFlag{false};
static const size_t AUDIO_FRAME_SAMPLES = 160;
static const size_t MAX_AUDIO_QUEUE_SIZE = 10;

#define SEG_SHIFT (4) 
#define QUANT_MASK (0xf)

static short seg_end[8] = { 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF };
static int search(int val, short* table, int size)
{
    int i;

    for (i = 0; i < size; i++) {
        if (val <= *table++)
            return (i);
    }
    return (size);
}

static unsigned char linear2alaw(int pcm_val) {
    int mask;
    int seg;
    unsigned char aval;

    if (pcm_val >= 0) {
        mask = 0xD5; /* sign (7th) bit = 1 */
    } else {
        mask = 0x55; /* sign bit = 0 */
        pcm_val = -pcm_val - 8;
    }

    seg = search(pcm_val, seg_end, 8);
    if (seg >= 8) /* out of range, return maximum value. */
        return (0x7F ^ mask);
    else {
        aval = seg << SEG_SHIFT;
        if (seg < 2)
            aval |= (pcm_val >> 4) & QUANT_MASK;
        else
            aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
        return (aval ^ mask);
    }
}

static int audio_g711a_encode(unsigned char g711_data[], const short amp[], int len)
{
    int i;

    for (i = 0; i < len; i++) {
        g711_data[i] = linear2alaw(amp[i]);
    }

    return len;
}

static int audio_PCMToG711a(char* InAudioData, char* OutAudioData, int DataLen)
{
    int Retaen = 0;

    if ((NULL == InAudioData) && (NULL == OutAudioData) && (0 == DataLen)) {
        LOG_DEBUG("Error, empty data or transmit failed, exit !\n");
        return -1;
    }

    Retaen = audio_g711a_encode((unsigned char*)OutAudioData, (short*)InAudioData, DataLen / 2);

    return Retaen;
}


static void audioCaptureThreadFunc() {
    LOG_INFO("Audio capture thread started\n");

    DeviceAudioInput* audioInput = MediaManager::GetInstance().GetDeviceAudioInput(DEV_AUDIO_INPUT_0);
    if (audioInput)
    {
        audioInput->Stop();
        sleep(1);
        audioInput->Start();
    }
        
    while (g_audioRunning) {
        IMPAudioFrame frame;

        int ret = audioInput->GetFrame(frame);
        if(ret != 0){
            LOG_INFO("GetFrame return fail\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        size_t frame_size = AUDIO_FRAME_SAMPLES/2;
        std::vector<uint8_t> pcmaFrame(frame_size);
        //LOG_INFO("frame size: %d\n", frame.len);
        int convertedLen = audio_PCMToG711a((char*)frame.virAddr, (char*)pcmaFrame.data(), frame.len);
        audioInput->ReleaseFrame(frame);
        //LOG_INFO("G711 frame size: %d\n", convertedLen);
        {
            std::lock_guard<std::mutex> lock(g_audioMutex);
            if (g_audioQueue.size() < MAX_AUDIO_QUEUE_SIZE) {
                g_audioQueue.push(std::move(pcmaFrame));
            } else {
                g_audioQueue.pop();
                g_audioQueue.push(std::move(pcmaFrame));
            }
            g_audioCV.notify_one();
        }
    }

    if (audioInput) 
        audioInput->Stop();
    
    LOG_INFO("Audio capture thread stopped\n");
}

int AudioFrameAcquireCallback(uint8_t* buffer, size_t* size) {
    std::lock_guard<std::mutex> lock(g_audioMutex);
    if (g_audioQueue.empty()) {
        //LOG_ERROR("g_audioQueue is null\n");
        return -1;
    }

    auto& front = g_audioQueue.front();
    size_t dataSize = front.size();
    if (*size < dataSize) {
        LOG_ERROR("dataSize(%d) > size(%d)\n", dataSize, *size);
        *size = 0;
        return -1;
    }

    //LOG_INFO("Audio capture pop queue\n");
    memcpy(buffer, front.data(), dataSize);
    *size = dataSize;
    //LOG_INFO("rtsp frame size: %d\n", front.size());

    g_audioQueue.pop();
    return 0;
}

void get_audio_data_thread_start(void)
{
    g_audioRunning = true;
    g_audioStopFlag = false;
    g_audioThread = std::thread(audioCaptureThreadFunc);
    g_audioThread.detach();
}

void get_audio_data_thread_stop(void)
{
    g_audioRunning = false;
    g_audioStopFlag = true;
}