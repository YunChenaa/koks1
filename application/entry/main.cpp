/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-11 17:19:22
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-29 14:31:58
 * @FilePath: /sense-koks/application/entry/main.cpp
 */

#include "LCDCameraPlayer.h"
#include <cstdint>
#include <cstdlib>
#include <thread>
#include <semaphore.h>
#define ENABLE_DEBUG_INFO

#include "faceunlock_wrapper.h"
#include "koks_project_def.h"
#include "koks1_config.h"
#include "factory.h"
#include "os_utils.h"
#include "partition_guard.h"
#include "senselock.h"
#include "st_koks_db.h"
#include "st_koks_led.h"
#include "st_koks_mediamanager.h"
#include "st_koks_uvc.h"
#include "sys_logger.h"
#include "st_koks_isp.h"
#include "sys_version.h"
#include "uart.h"
#include <chrono>
#ifdef KOKS_FEATURE_LCD_EMOTION
#include "LCDVedioPlayer.h"
#endif

using namespace st::sense_koks;

extern "C" {
    int get_bootup_timer(char *argv);
}
extern uint8_t g_uvc_init_video;
extern uint8_t g_sdk_verify;

int main(void)
{
    DEBUG_INFO("KOKS START");

    // std::string test_sn ="koks12345";
    // app_flash_write_sn((uint8_t *)test_sn.data(), test_sn.size());
    // std::string license = "36530F0DAA4367000BFF5332D5C66A248531E1B2C90963B127665051A82A8C53";
    // std::string license = "2A4572063187426A9E4345EC181DC80AEAB31E4E04583C6B3568D45E40A29351";
    // std::string license = "6676C205092851BC2D0A7AFEF096D270A75E79CFB783F8920E17F8E80F300400";
    // app_flash_write_licence((uint8_t*)license.data(), license.size());
    int ret = 0;
    // printf("                                 __         __       \n");
    // printf("   ________  ____  ________     / /______  / /_______\n");
    // printf("  / ___/ _ \\/ __ \\/ ___/ _ \\   / //_/ __ \\/ //_/ ___/\n");
    // printf(" (__  )  __/ / / (__  )  __/  / ,< / /_/ / ,< (__  ) \n");
    // printf("/____/\\___/_/ /_/____/\\___/  /_/|_|\\____/_/|_/____/  \n");
    // printf("\n\n");
    // printf("cpu %s sense koks compile at %s %s, version %s\n", GetCPUID().data(), __DATE__, __TIME__, SENSELOCK_V3X_VERSION); // 必须要打印出来，以便排查问题时核对版本
    // LibSerial::SerialPort serial_port;
    // serial_port.Open(SERIAL_PORT);
    // serial_port.SetBaudRate(LibSerial::BaudRate::BAUD_115200);
    // serial_port.SetCharacterSize(LibSerial::CharacterSize::CHAR_SIZE_8);
    // serial_port.SetParity(LibSerial::Parity::PARITY_NONE);
    // serial_port.SetStopBits(LibSerial::StopBits::STOP_BITS_1);

#if 0
    std::string version = std::string(SENSELOCK_V3X_VERSION);
    if (koks_config.log.path != "null") {
        if (SysLogger::GetInstance()->InitLogger((koks_config.log.path + "/test_" + version).c_str(), koks_config.log.level.level, koks_config.log.max_num,
                                                 koks_config.log.max_size)) {
            LOG_WARNING("sense koks compile at %s %s\n", __DATE__, __TIME__); // 必须要打印出来，以便排查问题时核对版本
        } else {
            std::cerr << "init log failed\n";
            return -1;
        }
    }
    else
#else
    {
        SysLogger::GetInstance()->SetLoggerLevel(koks_config.log.level.level);
    }
#endif
    LOG_INFO("cpu %s sense koks compile at %s %s, version %s\n", GetCPUID().data(), __DATE__, __TIME__, SENSELOCK_V3X_VERSION);

    PartitionGuard::Init();

    auto& db = koks_db::GetInstance();
    db.Init(koks_config.db.path);
    Senselock* sLock = Senselock::getInstance();
#ifdef KOKS_FEATURE_PALM
    sLock->init(608, 800);
#else
    sLock->init(480, 640);
#endif

#ifdef KOKS_FEATURE_LCD_EMOTION
    LCDVedioPlayer::getInstance()->playdefault();
#endif 

    auto t1 = GetCurTimeMs();
    FaceUnlockWrapper* faceUnlockWrapper = FaceUnlockWrapper::getInstance();
    if (faceUnlockWrapper->init()) {
        LOG_ERROR("faceSdk init failed\n");
        sLock->mState = MS_INVALID;
        sLock->mEarlyVerifyState = 2;
    }

    auto t2 = GetCurTimeMs();
    DEBUG_INFO("earlyGetFrame start");
    sLock->earlyGetFrame();
    DEBUG_INFO("earlyGetFrame done");

    st::sense_koks::LedProxy::Init();

    std::string serial_path = koks_config.serial.path;
    uint32_t serial_brate = koks_config.serial.BaudRate;
    auto& serial = Uart::GetInstance();
    serial.SetConfig(serial_path, Uart::int2rate(serial_brate));
    serial.Init();

    // serial.Listen();
    sLock->start();
    sLock->mHost->checkRlsKey();
    sLock->sendMessage(NID_READY, NULL, 0);
    DEBUG_INFO("READY");
#ifdef KOKS_FEATURE_LCD_CAMERA_PLAYER
    LCDCameraPlayer::getInstance();
 #endif   
    auto t3 = GetCurTimeMs();
    sLock->earlyVerify();
    auto t4 = GetCurTimeMs();
    DEBUG_INFO("earlyVerify done");
    LOG_INFO("TIME_COST: face init %llu, verify %llu\n", t2 - t1, t4 - t3);
#ifdef KOKS_FEATURE_WIFI
    void configure_net_ovd();
    configure_net_ovd();
#endif
    std::thread standby([&serial, &ret, sLock]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto sdk_verify = g_sdk_verify;
        LOG_INFO("g_FactoryTest = %d, sdk_verify = %u\n", g_FactoryTest, sdk_verify);

        if (sdk_verify == 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
#ifdef KOKS_FEATURE_VIDEO
        g_uvc_init_video = 1;
#endif
        auto& media_manager = MediaManager::GetInstance();

        if (sdk_verify != 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

#ifdef KOKS_FEATURE_AUDIO
        if (!g_FactoryTest) {
            ret = media_manager.Init(koks_config.media, MediaType::AUDIO);
            if (ret)
                LOG_ERROR("media_manager.Init failed with %d\n", ret);

            // ret = media_manager.Start(MediaType::AUDIO);
            // if (ret)
            //     LOG_ERROR("media_manager.Start failed with %d\n", ret);
        }
#endif

        if(g_FactoryTest) {
#ifdef KOKS_PROJECT_M53D
            system("devmem  0x10000040  32  0x08000007"); //usb device mode
#endif
            system("insmod /lib/modules/inlock_camera.ko");
            media_uvc_factory_init();
            LOG_INFO("sensen koks startup to factory test\n");
        }
#ifdef KOKS_FEATURE_VIDEO
        else {
#ifndef KOKS_FEATURE_UVC_MEDIA_STREAM_MODE
            system("insmod /lib/modules/inlock_camera.ko");
#endif
            Isp::GetInstance().WaitForBootDayNightJudge();
            media_uvc_init();
#ifndef KOKS_FEATURE_UVC_MEDIA_STREAM_MODE
            serial.cdc_Init("/dev/ttyGS0");
#endif
            LOG_INFO("sensen koks startup!\n");
        }
#endif
#ifdef KOKS_PROJECT_M53D
        void init_ovd_and_voip();
        init_ovd_and_voip();
#endif
        while (1) {
#ifdef IMAGE_CODEC_TEST
            static int index = 0;
            media_manager.GetDeviceVenc(0)->SaveJpeg(std::to_string(index++) + ".jpg");
            unsigned char* nv12Buffer = nullptr;
            int width, height;
            auto time_start = std::chrono::high_resolution_clock::now();
            st::sense_koks::SoftCodec::DecodeJPEGToNV12((std::to_string(index - 1) + ".jpg").c_str(), &nv12Buffer, &width, &height);
            auto time_end = std::chrono::high_resolution_clock::now();
            LOG_DEBUG("DecodeJPEGToNV12 cost %lld ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count());
            st::sense_koks::SoftCodec::SaveNV12ToFile((std::to_string(index - 1) + ".nv12").c_str(), nv12Buffer, width, height);
            free(nv12Buffer);
#endif
#ifdef KOKS_FEATURE_KDS
            if (!SenseControl::getInstance()->isFirstCmdReceived()) {
                serial.WritePipe('r'); // Send NID_READY
                std::this_thread::sleep_for(std::chrono::milliseconds(230));
            }
            else
#endif
            {
                // system("cat /proc/jz/audio/audio_info");
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    });
    standby.join();
    auto& media_manager = MediaManager::GetInstance();
    media_manager.Stop();
    media_manager.DeInit();
    serial.Deinit();
    koks_db::GetInstance().DeInit();
    return 0;
}
