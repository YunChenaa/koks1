#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <vector>
#include <string>

#include "koks1_config_types.h"
#include "sys_logger.h"
#include "st_koks_mediamanager.h"

extern "C" {
    #include <ws.h>
}


#define SAMPLE_RATE 8000
#define CHANNELS 1
#define BYTES_PER_SAMPLE 2
#define CHUNK_SIZE 320          // 20ms @ 8000Hz
#define WS_PORT 80

using namespace st::sense_koks;

static std::atomic<bool> g_transfer_enabled(false);   // transfer switch
static std::atomic<bool> g_running(false);            // module running flag

static std::queue<std::vector<uint8_t>> g_audio_queue;
static std::mutex g_queue_mutex;
static std::condition_variable g_queue_cv;

static std::thread g_send_thread;
static std::thread g_playback_thread;
static std::thread g_ws_thread;
static DeviceAudioInput* g_audio_input = nullptr;
static DeviceAudioOutput* g_audio_output = nullptr;

static struct ws_server g_ws_srv;

static void send_thread_func() {
    LOG_INFO("Send thread started\n");
    bool device_started = false;

    g_audio_input = MediaManager::GetInstance().GetDeviceAudioInput(DEV_AUDIO_INPUT_0);

    while (g_running) {
        if (g_transfer_enabled) {
            if (!device_started && g_audio_input) {
                if (g_audio_input->Start() == 0) {
                    device_started = true;
                    LOG_INFO("Audio input device started\n");
                } else {
                    LOG_ERROR("Failed to start audio input device\n");
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
            }

            if (device_started && g_audio_input) {
                IMPAudioFrame frame;
                int ret = g_audio_input->GetFrame(frame);
                if (ret == 0) {
                    size_t copy_len = (frame.len < CHUNK_SIZE) ? frame.len : CHUNK_SIZE;
                    ws_sendframe_bcast(WS_PORT,
                                       reinterpret_cast<const char*>(frame.virAddr),
                                       copy_len,
                                       WS_FR_OP_BIN);
                    g_audio_input->ReleaseFrame(frame);
                } else {
                    LOG_ERROR("GetFrame failed: %d\n", ret);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        } else {
            // Transfer disabled: stop device and sleep
            if (device_started && g_audio_input) {
                g_audio_input->Stop();
                device_started = false;
                LOG_INFO("Audio input device stopped\n");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    if (device_started && g_audio_input) {
        g_audio_input->Stop();
    }
    LOG_INFO("Send thread ended\n");
}

// ===== Playback thread =====
static void playback_thread_func() {
    LOG_INFO("Playback thread started\n");
    bool device_started = false;

    g_audio_output = MediaManager::GetInstance().GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0);

    while (g_running) {
        std::unique_lock<std::mutex> lock(g_queue_mutex);
        if (g_audio_queue.empty()) {
            g_queue_cv.wait_for(lock, std::chrono::milliseconds(100));
            continue;
        }

        if (!g_transfer_enabled) {
            // Flush queue
            LOG_DEBUG("Transfer disabled, flushing playback queue\n");
            std::queue<std::vector<uint8_t>> empty;
            std::swap(g_audio_queue, empty);
            lock.unlock();
            if (device_started && g_audio_output) {
                g_audio_output->Stop();
                device_started = false;
            }
            continue;
        }

        // Ensure playback device is started
        if (!device_started && g_audio_output) {
            if (g_audio_output->Start() == 0) {
                device_started = true;
                LOG_INFO("Audio output device started\n");
            } else {
                LOG_ERROR("Failed to start audio output device\n");
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
        }

        std::vector<uint8_t> data = std::move(g_audio_queue.front());
        g_audio_queue.pop();
        lock.unlock();

        IMPAudioFrame frm;
        frm.virAddr = reinterpret_cast<uint32_t*>(data.data());
        frm.len = data.size();
        int ret = g_audio_output->SendFrame(frm);
        if (ret != 0) {
            LOG_ERROR("SendFrame failed: %#x\n", ret);
        }
    }

    if (device_started && g_audio_output) {
        g_audio_output->Stop();
    }
    LOG_INFO("Playback thread ended\n");
}

// ===== WebSocket callbacks (extern "C") =====
extern "C" void onopen(ws_cli_conn_t client) {
    char *cli = ws_getaddress(client);
    char *port = ws_getport(client);
    LOG_INFO("Connection opened, addr: %s, port: %s\n", cli, port);
}

extern "C" void onclose(ws_cli_conn_t client) {
    char *cli = ws_getaddress(client);
    LOG_INFO("Connection closed, addr: %s\n", cli);
}

extern "C" void onmessage(ws_cli_conn_t client, const unsigned char *msg, uint64_t size, int type) {
    LOG_INFO("");

    if (type == WS_FR_OP_TXT) {
        std::string cmd(reinterpret_cast<const char*>(msg), size);
        if (cmd == "start") {
            g_transfer_enabled = true;
            LOG_INFO("Received 'start' command, transfer enabled\n");
        } else if (cmd == "stop") {
            g_transfer_enabled = false;
            LOG_INFO("Received 'stop' command, transfer disabled\n");
        } else {
            LOG_INFO("Unknown command: %s\n", cmd.c_str());
        }
        return;
    }

    // Binary audio data
    if (type == WS_FR_OP_BIN) {
        if (g_transfer_enabled) {
            std::vector<uint8_t> data(msg, msg + size);
            {
                std::lock_guard<std::mutex> lock(g_queue_mutex);
                g_audio_queue.push(std::move(data));
                g_queue_cv.notify_one();
            }
            LOG_INFO("Received audio data %zu bytes, enqueued\n", size);
        } else {
            LOG_INFO("Transfer disabled, dropped audio data %zu bytes\n", size);
        }
    }
}

int pcm_gateway_run(void) {

    LOG_INFO("Initializing PCM gateway...\n");
    if (g_running) {
        LOG_INFO("Gateway already running, no need to re-initialize\n");  // changed from LOG_WARN
        return 0;
    }


    g_running = true;
    g_transfer_enabled = true;

    //g_send_thread = std::thread(send_thread_func);
    g_playback_thread = std::thread(playback_thread_func);

    memset(&g_ws_srv, 0, sizeof(g_ws_srv));
    g_ws_srv.host = "0.0.0.0";
    g_ws_srv.port = WS_PORT;
    g_ws_srv.thread_loop = 0;
    g_ws_srv.timeout_ms = 5000;
    g_ws_srv.evs.onopen = onopen;
    g_ws_srv.evs.onclose = onclose;
    g_ws_srv.evs.onmessage = onmessage;

    g_ws_thread = std::thread([]() {
        LOG_INFO("WebSocket server thread started, port: %d\n", WS_PORT);
        ws_socket(&g_ws_srv);
        LOG_INFO("WebSocket server thread ended\n");
    });

    LOG_INFO("PCM gateway initialized successfully\n");
    return 0;
}


void pcm_gateway_cleanup(void) {
    if (!g_running) {
        return;
    }

    LOG_INFO("Starting PCM gateway cleanup...\n");
    g_running = false;
    g_transfer_enabled = false;

    g_queue_cv.notify_all();

    // Wait for threads to finish
    // if (g_send_thread.joinable()) {
    //     g_send_thread.join();
    // }

    if (g_playback_thread.joinable()) {
        g_playback_thread.join();
    }

    if (g_ws_thread.joinable()) {
        g_ws_thread.detach();
    }

    // Clear queue
    {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        std::queue<std::vector<uint8_t>> empty;
        std::swap(g_audio_queue, empty);
    }

    // Raw pointers: no need to delete (MediaManager manages lifecycle), just set to nullptr
    g_audio_input = nullptr;
    g_audio_output = nullptr;

    LOG_INFO("PCM gateway cleanup completed\n");
}