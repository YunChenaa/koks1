/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-13 22:46:13
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-03 11:40:55
 * @FilePath: /sense-koks/application/communication/src/sense_control.cpp
 */
#include "sense_control.h"
#include "crypto.h"
#include "senselock.h"
#include "senselock_handler.h"
#include "sys_logger.h"
#include "uart.h"
#include "updater.h"
// #include <os_event.h>
// #include <os_task.h>
// #include "file_utils.h"
// #include "os_mem.h"
// #include "ff.h"
#include "md5sum.h"
#include "os_utils.h"
#include "st_koks_db.h"
#include "rota_common.h"
#include <byteswap.h>
#include <cstdint>
#include <mutex>
#include <thread>
#include "koks_project_def.h"
#include "koks1_config.h"

// #include "wq_sec.h"
// #include "factory.h"
using namespace st::sense_koks;

#if 1
#define LOG_TAG "SenseControl"
#define PROTOCOL_KEY "Y6nkaNQkW+jAAUe="

static SenseControl* gSenseCtrl = NULL;

// #define bswap_16(a) ((((a >> 8)) & 0xFF) | (((a << 8)) & 0xFF00))

#define PROTOCOL_MID_OFFSET 2
#define PROTOCOL_SIZE_OFFSET 3
#define PROTOCOL_DATA_OFFSET 5
#define PROTOCOL_HEADER_SIZE 6
#define PROTOCOL_CMD_MAXSIZE (MAX_RECEIVE_DATA_SIZE - 3) // report exception if cmd data size check exceed the limitation

#define MAX_UART_MSG_SIZE 5

static uint16_t header_flag = 0xaaef;
static bool uartReceiveFlag = false;
static bool cdcReceiveFlag = false;
uint32_t msg_len = 0, enc_size = 0;
static uint8_t rcvBuf[MAX_RECEIVE_DATA_SIZE] = { 0 };
static uint8_t cdc_rcvBuf[MAX_RECEIVE_DATA_SIZE] = { 0 };
static uint8_t mUartMsgBuffer[MAX_UART_MSG_SIZE][MAX_RECEIVE_DATA_SIZE] = { 0 };

static uint8_t dec_rcvBuf[MAX_RECEIVE_DATA_SIZE] = { 0 };
static uint32_t rcvNum = 0;
static uint32_t start_rcvTime = 0;
static uint32_t end_rcvTime = 0;
static uint32_t cdc_rcvNum = 0;
static uint32_t cdc_start_rcvTime = 0;
static uint32_t cdc_end_rcvTime = 0;
static uint32_t msg_index = 0;

#define ONE_ENC_BLOCK_SIZE 16

static uint32_t current_time, last_time;
static void kindly_wait(int time_ms)
{
    current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    unsigned int time_diff = current_time - last_time;
    LOG_DEBUG("check time diff: current_time=%u last_time=%u diff=%u\n", current_time, last_time, time_diff);
    if (0 <= time_diff && time_diff < time_ms) {
        LOG_VERBOSE("need delay: current_time=%u last_time=%u diff=%u\n", (current_time), (last_time), (time_diff));
        std::this_thread::sleep_for(std::chrono::milliseconds(time_ms - time_diff));
    }
}

SenseControl* SenseControl::getInstance()
{
    if (gSenseCtrl == NULL) {
        gSenseCtrl = new SenseControl();
    }

    return gSenseCtrl;
}

SenseControl::SenseControl() : mListener(NULL), mEncryptMode(ST_ENCMODE_DEFAULT)
{
    // LOG_INFO("%s in", __func__);
    // set the default encryption key
    strncpy(mEncKey, PROTOCOL_KEY, strlen(PROTOCOL_KEY));
    memset(mDebugEncKey, 0, ENC_KEY_SIZE + 1);
    memset(savedSeedMd5, 0, sizeof(savedSeedMd5));
    init();
}

SenseControl::~SenseControl()
{
}

uint8_t SenseControl::checksum(uint8_t* buff, int size)
{
    uint8_t result = 0;
    for (int i = 0; i < size; i++) {
        result ^= buff[i];
    }

    return result;
}

#define CLEAR_FLAGS(a, b)                                                                                                                                                          \
    (a) = 0;                                                                                                                                                                       \
    (b) = 0;

void SenseControl::process_data(unsigned char* buf, uint16_t len)
{
    int dataLength = 0;
    if (uartReceiveFlag == false) {
        start_rcvTime = 0;
        start_rcvTime = GetCurTimeMs();
        uartReceiveFlag = true;
        memcpy(rcvBuf + rcvNum, buf, len);
    } else { // continue save a uart msg
        memcpy(rcvBuf + rcvNum, buf, len);
    }
    rcvNum += len;
parse_uart_data:
    if (rcvBuf[0] == 0xef) {
#ifdef KOKS_FEATURE_KDS
        if (!first_cmd_received) {
            if (rcvNum >= 3 && rcvBuf[1] == 0xaa && rcvBuf[2] != 0x00 && rcvBuf[2] != 0x01) {
                LOG_INFO("first_cmd_received\n");
                first_cmd_received = true;
            }
        }
#else
        first_cmd_received = true;
#endif
        // process the uart cmd
        if (mEncryptMode == ST_ENCMODE_AES) {
            LOG_VERBOSE("handling the ST_ENCMODE_AES cmd...\n");

            s_enc_msg* enc_msg = (s_enc_msg*)rcvBuf;
            if ((dataLength == 0) && (rcvNum >= sizeof(s_enc_msg))) {
                enc_size = bswap_16(enc_msg->size);
#ifdef KOKS_FEATURE_LCD_EMOTION
                if (enc_size == bswap_16(MID_INIT_ENCRYPTION) || enc_size== bswap_16(MID_EMOTION) || enc_size == bswap_16(MID_SET_RELEASE_ENC_KEY) || enc_size == bswap_16(MID_SET_DEBUG_ENC_KEY)) {
#else
                if (enc_size == bswap_16(MID_INIT_ENCRYPTION) || enc_size == bswap_16(MID_SET_RELEASE_ENC_KEY) || enc_size == bswap_16(MID_SET_DEBUG_ENC_KEY)) {
#endif // DEBUG
                    LOG_INFO("mid %#x, enc_size is %d, it is a special cmd, no need to decrypt\n", enc_msg->size, enc_size);
                    mListener->onMsg(*(s_msg*)&rcvBuf[2]);
                    rcvNum = 0;
                    uartReceiveFlag = false;
                    dataLength = 0;
                    memset(rcvBuf, 0, sizeof(rcvBuf));
                    return;
                }
                dataLength = enc_size + sizeof(s_enc_msg) + 1;
                if (dataLength > PROTOCOL_CMD_MAXSIZE) {
                    LOG_WARNING("CMD size exceed limitation %d\n", dataLength);
                    CLEAR_FLAGS(rcvNum, uartReceiveFlag);
                }
            }
            if ((rcvNum >= dataLength) && (rcvNum != 0) && (dataLength != 0)) {
                if (checksum(enc_msg->data, enc_size) == ((uint8_t*)(enc_msg))[sizeof(s_enc_msg) + enc_size]) {
                    LOG_DEBUG("check xor success\n");
                    memset(dec_rcvBuf, 0, sizeof(dec_rcvBuf));
                    decrypt(enc_msg->data, enc_size, dec_rcvBuf);
                    mListener->onMsg(*(s_msg*)(dec_rcvBuf));
                } else {
                    LOG_ERROR("check xor failed\n");
                }
                if (rcvNum > dataLength) {
                    LOG_WARNING("store an other uart cmd, size %d\n", rcvNum - dataLength);
                    memcpy(rcvBuf, rcvBuf + dataLength, rcvNum - dataLength);
                    rcvNum = rcvNum - dataLength;
                    dataLength = 0;
                    uartReceiveFlag = true;
                    goto parse_uart_data; // parse the data again
                } else {
                    rcvNum = 0;
                    uartReceiveFlag = false;
                    dataLength = 0;
                    memset(rcvBuf, 0, sizeof(rcvBuf));
                }
            }
        } else if (mEncryptMode == ST_ENCMODE_SMPL) {
            LOG_DEBUG("handling the ST_ENCMODE_SMPL cmd...\n");

            s_enc_msg* enc_msg = (s_enc_msg*)rcvBuf;
            enc_size = bswap_16(enc_msg->size);
            if (checksum(enc_msg->data, enc_size) == ((uint8_t*)(enc_msg))[sizeof(s_enc_msg) + enc_size]) {
                memset(dec_rcvBuf, 0, sizeof(dec_rcvBuf));
                decBytes(enc_msg->data, enc_size, (uint8_t*)mEncKey, 16, dec_rcvBuf);
                mListener->onMsg(*(s_msg*)(dec_rcvBuf));
            } else {
                LOG_ERROR("check xor failed\n");
            }

        } else {
            // LOG_INFO("handling the ST_ENCMODE_DEFAULT cmd...\n");
            if (dataLength == 0 && rcvNum >= 5) {
                dataLength = ((rcvBuf[3] << 8) | rcvBuf[4]) + sizeof(s_msg) + 3;
                LOG_DEBUG("The length of protocal(include head) is 0x%x\n", dataLength);
                if (dataLength > PROTOCOL_CMD_MAXSIZE) {
                    LOG_WARNING("CMD size exceed limitation %d\n", dataLength);
                    CLEAR_FLAGS(rcvNum, uartReceiveFlag);
                    return;
                }
            }
            // LOG_INFO("rcvNum %d, dataLength %d\n", rcvNum, dataLength);
            if ((rcvNum >= dataLength) && (rcvNum != 0) && (dataLength != 0)) {
                if (checksum(&rcvBuf[2], dataLength - 3) == rcvBuf[dataLength - 1]) {
                    if (mListener)
                        mListener->onMsg(*(s_msg*)&rcvBuf[2]);
                    else
                        LOG_ERROR("uart msg has no listener !\n");
                } else {
                    LOG_ERROR("Checksum is error!\n");
                }
                if (rcvNum > dataLength) {
                    LOG_WARNING("store an other uart cmd, size %d\n", rcvNum - dataLength);
                    memcpy(rcvBuf, rcvBuf + dataLength, rcvNum - dataLength);
                    rcvNum = rcvNum - dataLength;
                    dataLength = 0;
                    uartReceiveFlag = true;
                    goto parse_uart_data; // parse the data again
                } else {
                    rcvNum = 0;
                    uartReceiveFlag = false;
                    dataLength = 0;
                    memset(rcvBuf, 0, sizeof(rcvBuf));
                }
            }
        }
        // if uart msg is timeout, clear the buffer
        end_rcvTime = GetCurTimeMs();
        if (end_rcvTime - start_rcvTime > 1000) {
            uartReceiveFlag = false;
            dataLength = 0;
            rcvNum = 0;
            memset(rcvBuf, 0, sizeof(rcvBuf));
        }
    } else {
        LOG_ERROR("uart msg header is wrong, clean the buffer!\n");
        uartReceiveFlag = false;
        dataLength = 0;
        rcvNum = 0;
        memset(rcvBuf, 0, sizeof(rcvBuf));
    }
    return;
}

void SenseControl::cdc_process_data(unsigned char* buf, uint16_t len)
{
    UpdaterHandler* sUpdater_handler = UpdaterHandler::getInstance();
    SenselockHandler* sLock_handler = SenselockHandler::getInstance();
    int dataLength = 0;
    if (cdcReceiveFlag == false) {
        cdc_start_rcvTime = 0;
        cdc_start_rcvTime = GetCurTimeMs();
        cdcReceiveFlag = true;
        memcpy(cdc_rcvBuf + cdc_rcvNum, buf, len);
    } else { // continue save a uart msg
        memcpy(cdc_rcvBuf + cdc_rcvNum, buf, len);
    }
    cdc_rcvNum += len;
parse_uart_data:
    if (cdc_rcvBuf[0] == 0xef) {
        if (dataLength == 0 && cdc_rcvNum >= 5) {
            dataLength = ((cdc_rcvBuf[3] << 8) | cdc_rcvBuf[4]) + sizeof(s_msg) + 3;
            LOG_DEBUG("The length of protocal(include head) is 0x%x\n", dataLength);
            if (dataLength > PROTOCOL_CMD_MAXSIZE) {
                LOG_WARNING("CMD size exceed limitation %d\n", dataLength);
                CLEAR_FLAGS(cdc_rcvNum, cdcReceiveFlag);
                return;
            }
        }
        // LOG_INFO("rcvNum %d, dataLength %d\n", rcvNum, dataLength);
        if ((cdc_rcvNum >= dataLength) && (cdc_rcvNum != 0) && (dataLength != 0)) {
            if (checksum(&cdc_rcvBuf[2], dataLength - 3) == cdc_rcvBuf[dataLength - 1]) {
                s_msg* msg = reinterpret_cast<s_msg*>(&cdc_rcvBuf[2]);
                switch (msg->mid) {
                case MID_SET_UTC: {
                    LOG_INFO("cdc_process_data MID_CDC_TIME_SYNC\n");
                    cam_msg_sync_time_data* data = (cam_msg_sync_time_data*)msg->data;
                    uint32_t second_elapse = data->timestamp[0] << 24 | data->timestamp[1] << 16 | data->timestamp[2] << 8 | data->timestamp[3];
                    Senselock::setUTCTime(second_elapse);
                    Senselock::getInstance()->reply(msg->mid, MR_SUCCESS, nullptr, 0, s_type::TYPE_CDC);
                } break;
                case MID_GET_VERSION: {
                    LOG_INFO("cdc_process_data MID_CDC_GET_VERSION\n");
                    auto version = rota_get_version();
                    s_msg_reply_version_data version_data = { 0 };
                    strncpy((char*)&version_data.version_info, version, sizeof(version_data.version_info) - 1);
                    Senselock::getInstance()->reply(MID_GET_VERSION, MR_SUCCESS, &version_data, sizeof(s_msg_reply_version_data), s_type::TYPE_CDC);
                } break;
                case MID_START_OTA: {
                    LOG_INFO("cdc_process_data MID_START_OTA\n");
                    sUpdater_handler->startOTA(s_type::TYPE_CDC);
                    break;
                }
                case MID_GET_OTA_STATUS: {
                    LOG_INFO("cdc_process_data MID_GET_OTA_STATUS\n");
                    sUpdater_handler->getOTAStatus(s_type::TYPE_CDC);
                    break;
                }
                case MID_OTA_HEADER: {
                    LOG_INFO("cdc_process_data MID_OTA_HEADER\n");
                    sUpdater_handler->handleMsgOTAHeader(msg, s_type::TYPE_CDC);
                    break;
                }
                case MID_STOP_OTA: {
                    LOG_INFO("cdc_process_data MID_STOP_OTA\n");
                    sUpdater_handler->stopOTA(s_type::TYPE_CDC);
                    break;
                }
                case MID_OTA_PACKET: {
                    // LOG_INFO("cdc_process_data MID_OTA_PACKET\n");
                    sUpdater_handler->OTAPacketTransmit(msg, s_type::TYPE_CDC);
                    break;
                }
                case MID_CONFIG_BAUDRATE: {
                    LOG_INFO("cdc_process_data MID_CONFIG_BAUDRATE\n");
                    s_note_config_baudrate* config_baudrate_t = (s_note_config_baudrate*)(msg->data);
                    Senselock::getInstance()->reply(MID_CONFIG_BAUDRATE, MR_SUCCESS, NULL, 0, s_type::TYPE_CDC);
                    break;
                }
#ifdef KOKS_FEATURE_VIDEO
                case MID_SET_UVC_SPECIFIC_FRAME: {
                    LOG_INFO("cdc_process_data MID_SET_UVC_SPECIFIC_FRAME\n");
                    sLock_handler->handleSetUVCStreamType(msg, s_type::TYPE_CDC);
                    break;
                }
                case MID_SET_VIDEO_PARAM: {
                    LOG_INFO("cdc_process_data MID_SET_VIDEO_PARAM\n");
                    sLock_handler->handleSetVideoParam(msg, s_type::TYPE_CDC);
                    break;
                }
                case MID_UVC_SWITCH: {
                    LOG_INFO("cdc_process_data MID_UVC_SWITCH\n");
                    sLock_handler->handleUVCSwitch(msg, s_type::TYPE_CDC);
                    break;
                }
                case MID_UVC_SWITCH2: {
                    LOG_INFO("cdc_process_data MID_UVC_SWITCH2\n");
                    sLock_handler->handleUVCSwitch2(msg);
                    break;
                }
                case MID_GET_VIDEO_MODE: {
                    sLock_handler->handleGetVideoMode(msg, s_type::TYPE_CDC);
                    break;
                }
#endif
                default: {
                    LOG_ERROR("cdc_process_data: unknown msg id %d\n", msg->mid);
                    break;
                }
                }
            } else {
                LOG_ERROR("Checksum is error!\n");
            }
            if (cdc_rcvNum > dataLength) {
                LOG_WARNING("store an other uart cmd, size %d\n", cdc_rcvNum - dataLength);
                memcpy(cdc_rcvBuf, cdc_rcvBuf + dataLength, cdc_rcvNum - dataLength);
                cdc_rcvNum = cdc_rcvNum - dataLength;
                dataLength = 0;
                cdcReceiveFlag = true;
                goto parse_uart_data; // parse the data again
            } else {
                cdc_rcvNum = 0;
                cdcReceiveFlag = false;
                dataLength = 0;
                memset(cdc_rcvBuf, 0, sizeof(cdc_rcvBuf));
            }
        }

        // if uart msg is timeout, clear the buffer
        cdc_end_rcvTime = GetCurTimeMs();
        if (cdc_end_rcvTime - cdc_start_rcvTime > 1000) {
            cdcReceiveFlag = false;
            dataLength = 0;
            cdc_rcvNum = 0;
            memset(cdc_rcvBuf, 0, sizeof(cdc_rcvBuf));
        }
    } else {
        LOG_ERROR("cdc msg header is wrong, clean the buffer!\n");
        cdcReceiveFlag = false;
        dataLength = 0;
        cdc_rcvNum = 0;
        memset(cdc_rcvBuf, 0, sizeof(cdc_rcvBuf));
    }
    return;
}

#if 0
void SenseControl::cdc_process_data(unsigned char* buf, uint16_t len)
{
    UpdaterHandler* sUpdater_handler = UpdaterHandler::getInstance();
    if (buf[0] == 0xef && buf[1] == 0xaa) {
        int dataLength = ((buf[3] << 8) | buf[4]) + sizeof(s_msg) + 3;
        if (!checksum(&buf[2], dataLength - 3) == buf[dataLength - 1]) {
            LOG_ERROR("cdc checksum is error,dataLength %d,checksum %#x, read checksum %#x\n", dataLength, checksum(&buf[2], dataLength - 3), buf[dataLength - 1]);
            for (int i = 0; i < len; i++) {
                printf("buf[%d] = %#x\n", i, buf[i]);
            }
            printf("\n");
            return;
        }
        for(int i=0;i<20;i++) {
            printf("%#x ", buf[i]);
        }
        printf("\n");
        LOG_INFO("cdc_process_data: len %d, dataLength %d\n", len, dataLength);
        s_msg* msg = reinterpret_cast<s_msg*>(&buf[2]);
        switch (msg->mid) {
        case MID_SET_UTC: {
            LOG_INFO("cdc_process_data MID_CDC_TIME_SYNC\n");
            cam_msg_sync_time_data* data = (cam_msg_sync_time_data*)msg->data;
            uint32_t second_elapse = data->timestamp[0] << 24 | data->timestamp[1] << 16 | data->timestamp[2] << 8 | data->timestamp[3];
            Senselock::setUTCTime(second_elapse);
            Senselock::getInstance()->reply(msg->mid, MR_SUCCESS, nullptr, 0, s_type::TYPE_CDC);
            break;
        }
        case MID_GET_VERSION: {
            LOG_INFO("cdc_process_data MID_CDC_GET_VERSION\n");
            std::string version = std::string(SENSELOCK_V3X_VERSION);
            s_msg_reply_version_data version_data = { 0 };
            strncpy((char*)&version_data.version_info, version.c_str(), sizeof(version_data.version_info) - 1);
            Senselock::getInstance()->reply(MID_GET_VERSION, MR_SUCCESS, &version_data, sizeof(s_msg_reply_version_data), s_type::TYPE_CDC);
            break;
        }
        case MID_START_OTA: {
            LOG_INFO("cdc_process_data MID_START_OTA\n");
            sUpdater_handler->startOTA(s_type::TYPE_CDC);
            break;
        }
        case MID_GET_OTA_STATUS: {
            LOG_INFO("cdc_process_data MID_GET_OTA_STATUS\n");
            sUpdater_handler->getOTAStatus(s_type::TYPE_CDC);
            break;
        }
        case MID_OTA_HEADER: {
            LOG_INFO("cdc_process_data MID_OTA_HEADER\n");
            sUpdater_handler->handleMsgOTAHeader(msg, s_type::TYPE_CDC);
            break;
        }
        case MID_STOP_OTA: {
            LOG_INFO("cdc_process_data MID_STOP_OTA\n");
            sUpdater_handler->stopOTA(s_type::TYPE_CDC);
            break;
        }
        case MID_OTA_PACKET: {
            LOG_INFO("cdc_process_data MID_OTA_PACKET\n");
            sUpdater_handler->OTAPacketTransmit(msg, s_type::TYPE_CDC);
            break;
        }
        case MID_CONFIG_BAUDRATE: {
            LOG_INFO("cdc_process_data MID_CONFIG_BAUDRATE\n");
            s_note_config_baudrate* config_baudrate_t = (s_note_config_baudrate*)(msg->data);
            Senselock::getInstance()->reply(MID_CONFIG_BAUDRATE, MR_SUCCESS, NULL, 0, s_type::TYPE_CDC);
            break;
        }
        default: {
            LOG_ERROR("cdc_process_data: unknown msg id %d\n", msg->mid);
            break;
        }
        }
    } else {
        LOG_ERROR("cdc msg header is wrong, clean the buffer!\n");
        return;
    }
}
#endif

void SenseControl::sendReadyMessage()
{
    if (!first_cmd_received) {
        Senselock::getInstance()->sendMessage(NID_READY, nullptr, 0);
    }
}

s_status SenseControl::init(void)
{
    int ret = OK;
    crypto::GetInstance().Init();
    has_rls_key = false;
    has_dbg_key = false;
    // init mutex lock, queue, event
    mUartMSGQueue.clear();
    return (s_status)ret;
}

s_status SenseControl::deinit(void)
{
    return OK;
}

s_status SenseControl::config_uart(uint32_t baudrate)
{
    Uart::GetInstance().SetBaudrate(Uart::int2rate(static_cast<uint32_t>(baudrate)));
    return OK;
}

uint8_t buf[(((MAX_SEND_IMAGE_SIZE >> 4) + 1) << 4) + PROTOCOL_HEADER_SIZE];
uint8_t enc_buf[(((MAX_SEND_IMAGE_SIZE >> 4) + 1) << 4) + PROTOCOL_HEADER_SIZE];

void SenseControl::sense_uart_send(uint8_t* send_buffer, uint32_t send_len)
{
    Uart::GetInstance().WriteByteSequence(send_buffer, send_len);
}

void SenseControl::sense_cdc_send(uint8_t* send_buffer, uint32_t send_len)
{
    Uart::GetInstance().cdc_WriteByteSequence(send_buffer, send_len);
}

void SenseControl::send(uint8_t mid, void* data, uint16_t data_size, int max_buf_size, s_type type)
{

    int total_size = sizeof(header_flag) + sizeof(s_msg) + data_size + 1;

    std::lock_guard<std::mutex> lock(send_mutex);
    // LOG_INFO("send: mid %d, data_size %d, max_buf_size %d, type %d\n", mid, data_size, max_buf_size, type);
    /*
     *  [0xEF 0xAA msg.id msg.size msg.data[msg.size] checksum]
     *             |<-----------msg_body_size------>|
     */
    memset(&(buf), 0, sizeof(buf));
    memset(&(enc_buf), 0, sizeof(enc_buf));

    s_msg* msg = (s_msg*)(buf + PROTOCOL_MID_OFFSET);
    *(uint16_t*)buf = header_flag;
    msg->mid = mid;
    *(uint16_t*)(&msg->size_heb) = bswap_16(data_size);

    s_enc_msg* enc_msg = (s_enc_msg*)enc_buf;
    *(uint16_t*)enc_buf = header_flag;
    uint16_t enc_size = sizeof(s_msg) + data_size;

    // all msg M->H have data
    if (data_size && data != nullptr) {
        LOG_VERBOSE("msg->mid %d, data_size %d, max_buf_size %d mEncryptMode %d\n", msg->mid, data_size, max_buf_size, mEncryptMode);
        for (int i = 0; i < data_size; i++) {
            uint8_t* p = (uint8_t*)data;
            // LOG_DEBUG("data[%d]=0x%#x\n",i,p[i]);
        }
        if (mEncryptMode == ST_ENCMODE_AES && type == s_type::TYPE_UART) {
            memcpy(msg->data, data, data_size);
            LOG_VERBOSE("before encrypted sizeof(s_msg) %d\n", sizeof(s_msg));
            enc_size = ((enc_size >> 4) + 1) << 4; // 16byte aligned
            encrypt(&msg->mid, sizeof(s_msg) + data_size, enc_msg->data);
            LOG_VERBOSE("after encrypted\n");
            enc_msg->size = bswap_16(enc_size);
            total_size = enc_size + sizeof(s_enc_msg) + 1;
        } else if (mEncryptMode == ST_ENCMODE_SMPL && type == s_type::TYPE_UART) {
            msg = (s_msg*)enc_msg->data;
            msg->mid = mid;
            *(uint16_t*)(&msg->size_heb) = bswap_16(data_size);
            memcpy(msg->data, data, data_size);
            encBytes(&msg->mid, sizeof(s_msg) + data_size, (uint8_t*)mEncKey, 16);
            enc_msg->size = bswap_16(enc_size);
            total_size = enc_size + sizeof(s_enc_msg) + 1;
        } else {
            memcpy(msg->data, data, data_size);
        }
    }

    // calculate checksum
    if (mEncryptMode && type == s_type::TYPE_UART) {
        enc_buf[total_size - 1] = checksum((uint8_t*)enc_msg->data, enc_size);
        // some kind of host can not response too close instant msg, this is a kindly_wait
        if(mid != MID_OTA_PACKET) {
            kindly_wait(50);
        }

        sense_uart_send(enc_buf, total_size);
        LOG_VERBOSE("after encrypted\n");
    } else {
        // remove header(2byte) + checksum(1byte)
        buf[total_size - 1] = checksum((uint8_t*)msg, total_size - 3);
        // some kind of host can not response too close instant msg, this is a kindly_wait
        if(mid != MID_OTA_PACKET) {
            kindly_wait(50);
        }
        if (type == s_type::TYPE_CDC) {
            sense_cdc_send(buf, total_size);
        } else if (type == s_type::TYPE_UART) {
            sense_uart_send(buf, total_size);
        }
    }
    last_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return;
}


void SenseControl::sendNoEncrypt(uint8_t mid, void* data, uint16_t data_size, int max_buf_size, s_type type )
{
    LOG_DEBUG("sendNoEncrypt: mid %d, data_size %d, max_buf_size %d, type %d\n", mid, data_size, max_buf_size, type);
    int total_size = sizeof(header_flag) + sizeof(s_msg) + data_size + 1;
    /*
     *  [0xEF 0xAA msg.id msg.size msg.data[msg.size] checksum]
     *             |<-----------msg_body_size------>|
     */
    memset(&(buf), 0, sizeof(buf));

    s_msg* msg = (s_msg*)(buf + PROTOCOL_MID_OFFSET);
    *(uint16_t*)buf = header_flag;
    msg->mid = mid;
    *(uint16_t*)(&msg->size_heb) = bswap_16(data_size);

    // all msg M->H have data
    if (data_size && data != nullptr) {
        LOG_VERBOSE("msg->mid %d, data_size %d, max_buf_size %d mEncryptMode %d\n", msg->mid, data_size, max_buf_size, mEncryptMode);
        for (int i = 0; i < data_size; i++) {
            uint8_t* p = (uint8_t*)data;
            // LOG_DEBUG("NoEdata[%d]=0x%#x\n",i,p[i]);
        }
        memcpy(msg->data, data, data_size);
    }

    // calculate checksum
    // remove header(2byte) + checksum(1byte)
    buf[total_size - 1] = checksum((uint8_t*)msg, total_size - 3);
    // some kind of host can not response too close instant msg, this is a kindly_wait
    if(mid != MID_OTA_PACKET) {
        kindly_wait(50);
    }
    if (type == s_type::TYPE_CDC) {
        sense_cdc_send(buf, total_size);
    } else if (type == s_type::TYPE_UART) {
        sense_uart_send(buf, total_size);
    }

    last_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return;
}
void SenseControl::sendMsg(s_msg_note_data* smsg, uint16_t note_data_size, s_type type)
{
    send(MID_NOTE, (void*)smsg, note_data_size, MAX_NOTE_DATA_SIZE, type);
    return;
}

void SenseControl::sendReply(s_msg_reply_data* rmsg, uint16_t reply_data_size, s_type type)
{
    #if !defined(KOKS_FEATURE_KDS)
    if (rmsg->mid == MID_SET_DEBUG_ENC_KEY || rmsg->mid == MID_SET_RELEASE_ENC_KEY) {
        LOG_INFO("sendReply: reply_data_size %d, mid %#x\n", reply_data_size, rmsg->mid);
        EncMode mEncmode = Senselock::getInstance()->mHost->getEncryptionMode();
        LOG_INFO("sendReply: mEncmode %d\n", mEncmode);
        Senselock::getInstance()->mHost->setEncryptionMode(ST_ENCMODE_DEFAULT);
        send(MID_REPLY, (void*)rmsg, reply_data_size, MAX_REPLY_DATA_SIZE, type);
        Senselock::getInstance()->mHost->setEncryptionMode(mEncmode);
    } else {
        send(MID_REPLY, (void*)rmsg, reply_data_size, MAX_REPLY_DATA_SIZE, type);
	}
    #else //KOKS_FEATURE_KDS
        send(MID_REPLY, (void*)rmsg, reply_data_size, MAX_REPLY_DATA_SIZE, type);
    #endif // KOKS_FEATURE_KDS
}

void SenseControl::sendReplyNoEncrypt(s_msg_reply_data* rmsg, uint16_t reply_data_size, s_type type)
{
    if (rmsg->mid == MID_SET_DEBUG_ENC_KEY || rmsg->mid == MID_SET_RELEASE_ENC_KEY) {
        LOG_INFO("sendReply: reply_data_size %d, mid %#x\n", reply_data_size, rmsg->mid);
        EncMode mEncmode = Senselock::getInstance()->mHost->getEncryptionMode();
        LOG_INFO("sendReply: mEncmode %d\n", mEncmode);
        Senselock::getInstance()->mHost->setEncryptionMode(ST_ENCMODE_DEFAULT);
        send(MID_REPLY, (void*)rmsg, reply_data_size, MAX_REPLY_DATA_SIZE, type);
        Senselock::getInstance()->mHost->setEncryptionMode(mEncmode);
    } else {
        sendNoEncrypt(MID_REPLY, (void*)rmsg, reply_data_size, MAX_REPLY_DATA_SIZE, type);
	}
}

void SenseControl::sendImg(unsigned char* image, uint16_t image_size)
{
    send(MID_IMAGE, (void*)image, image_size, MAX_SEND_IMAGE_SIZE);
    return;
}

s_status SenseControl::setKey(uint8_t* seed, int size)
{
    LOG_INFO("start to set encryption key\n");
    uint8_t key_number[ENC_KEY_SIZE + strlen(ENC_KEY_HEADER) + 1] = { 0 };
    uint8_t key_tmp[5] = { 0 };
    // the input seed must be 4 bytes
    if (size != 4) {
        LOG_ERROR("Incorrect input seed size\n");
        return INVALID_PARAM;
    }
    memcpy((char*)key_tmp, (char*)seed, size);
    // TODO:calculate the real encryption key
    unsigned char digest[16], md5str[33];
    MD5_CTX md5c;
    MD5Init(&md5c); //初始化
    MD5Update(&md5c, key_tmp, size);
    MD5Final(digest, &md5c);
    MD5HexToStr(md5str, digest);
    LOG_DEBUG("md5sum: str = %s\n", md5str);

    // get the key_number for encrypt
    getKeyNumber(key_number);

    // make the encKey
    snprintf((char*)mEncKey, ENC_KEY_SIZE + 1, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", md5str[key_number[0]], md5str[key_number[1]], md5str[key_number[2]], md5str[key_number[3]],
             md5str[key_number[4]], md5str[key_number[5]], md5str[key_number[6]], md5str[key_number[7]], md5str[key_number[8]], md5str[key_number[9]], md5str[key_number[10]],
             md5str[key_number[11]], md5str[key_number[12]], md5str[key_number[13]], md5str[key_number[14]], md5str[key_number[15]]);

    LOG_DEBUG("mEncKey:%s\n", mEncKey);
    memcpy(savedSeedMd5, md5str, 32);
    return OK;
}

s_status SenseControl::checkRlsKey()
{
    has_rls_key = hasEncKey(RELEASE_ENC_KEY, rls_key_number);
    return OK;
}

s_status SenseControl::ReConfigureKeyFromSavedMd5()
{
    uint8_t key_number[ENC_KEY_SIZE + strlen(ENC_KEY_HEADER) + 1] = { 0 };

    // get the key_number for encrypt
    getKeyNumber(key_number);

    // make the encKey
    snprintf((char*)mEncKey, ENC_KEY_SIZE + 1, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", savedSeedMd5[key_number[0]], savedSeedMd5[key_number[1]], savedSeedMd5[key_number[2]],
             savedSeedMd5[key_number[3]], savedSeedMd5[key_number[4]], savedSeedMd5[key_number[5]], savedSeedMd5[key_number[6]], savedSeedMd5[key_number[7]],
             savedSeedMd5[key_number[8]], savedSeedMd5[key_number[9]], savedSeedMd5[key_number[10]], savedSeedMd5[key_number[11]], savedSeedMd5[key_number[12]],
             savedSeedMd5[key_number[13]], savedSeedMd5[key_number[14]], savedSeedMd5[key_number[15]]);

    LOG_DEBUG("mEncKey:%s\n", mEncKey);
    return OK;
}

bool SenseControl::checkKeyNumberLegal(uint8_t* key_number)
{
    // check key number, must be in 0-31
    for (int index = 0; index < ENC_KEY_SIZE; index++) {
        LOG_DEBUG("input key index:%d value:0x%x\n", index, key_number[index]);
        if (key_number[index] > 32 || key_number[index] < 0) {
            LOG_ERROR("Incorrect input key index:%d value:0x%x\n", index, key_number[index]);
            return false;
        }
    }
    return true;
}

s_status SenseControl::setKeyNumber(uint8_t* key_number, uint8_t key_type, bool enforce)
{
    s_status ret = OK;
    uint8_t tmp_index[ENC_KEY_SIZE + strlen(ENC_KEY_HEADER) + 1] = { 0 };
    // check key number, must be in 0-31
    for (int index = 0; index < ENC_KEY_SIZE; index++) {
        LOG_DEBUG("input key index:%d value:0x%x\n", index, key_number[index]);
        if (key_number[index] > 32 || key_number[index] < 0) {
            LOG_ERROR("Incorrect input key index:%d value:0x%x\n", index, key_number[index]);
            return INVALID_PARAM;
        }
    }
    if (key_type == DEBUG_ENC_KEY) {
        memcpy(mDebugEncKey, key_number, ENC_KEY_SIZE);
        has_dbg_key = true;
        return OK;
    } else if (key_type == RELEASE_ENC_KEY) {
        auto& db = st::sense_koks::koks_db::GetInstance();
        st::sense_koks::KeyStrategy keyStrategy;
        if (!has_rls_key || enforce) {
            // save release key_number
            st::sense_koks::koks_db::GetInstance().writeData(&keyStrategy, (const void*)key_number);
            // encrypt the key_number
            memcpy(rls_key_number, key_number, ENC_KEY_SIZE);
            has_rls_key = true;
            ret = OK;

        } else {
            LOG_WARNING("release key is already exist !\n");
            ret = EXCEED_LIMITATION; // Release already exists, so rejected
        }
    } else {
        LOG_ERROR("not support key_type:%d\n", key_type);
        return INVALID_PARAM;
    }

    return ret;
}

bool SenseControl::hasEncKey(uint8_t key_type, uint8_t* key_number)
{
    int ret = false;
    if (key_type == RELEASE_ENC_KEY) {
        auto& db = st::sense_koks::koks_db::GetInstance();
        std::vector<uint8_t> key;
        st::sense_koks::KeyStrategy keyStrategy;
        if (db.readData(&keyStrategy, &key) == ST_ERRNO_SUCCESS) {
            memcpy(key_number, key.data(), ENC_KEY_SIZE);
            LOG_INFO("Read release key success!\n");
            return true;
        }
    } else if (key_type == DEBUG_ENC_KEY) {
        if (has_dbg_key) {
            memcpy(key_number, mDebugEncKey, ENC_KEY_SIZE);
            LOG_INFO("Read debug key success!\n");
            return true;
        }
    }
    LOG_WARNING("No key found\n");
    return false;
}

s_status SenseControl::getKeyNumber(uint8_t* key_number)
{
    if (has_rls_key) {
        memcpy(key_number, rls_key_number, ENC_KEY_SIZE);
        LOG_DEBUG("Use release key\n");
    } else if (hasEncKey(DEBUG_ENC_KEY, key_number)) {
        LOG_DEBUG("Use debug key");
    } else {
#ifdef KOKS_FEATURE_DSM
        key_number[0] = 0;
        key_number[1] = 1;
        key_number[2] = 2;
        key_number[3] = 3;
        key_number[4] = 4;
        key_number[5] = 5;
        key_number[6] = 6;
        key_number[7] = 7;
        key_number[8] = 8;
        key_number[9] = 9;
        key_number[10] = 10;
        key_number[11] = 11;
        key_number[12] = 12;
        key_number[13] = 13;
        key_number[14] = 14;
        key_number[15] = 15;
#else
        key_number[0] = 6;
        key_number[1] = 18;
        key_number[2] = 7;
        key_number[3] = 3;
        key_number[4] = 13;
        key_number[5] = 13;
        key_number[6] = 23;
        key_number[7] = 4;
        key_number[8] = 8;
        key_number[9] = 1;
        key_number[10] = 0;
        key_number[11] = 25;
        key_number[12] = 9;
        key_number[13] = 2;
        key_number[14] = 2;
        key_number[15] = 7;
#endif
        LOG_INFO("Using Compatible Key\n");
    }

    return OK;
}

s_status SenseControl::encBytes(uint8_t* bytes, int length, const uint8_t* key, int stride, uint8_t* out)
{
    if (!out)
        out = bytes;

    for (int index = 0; index < length; index++) {
        out[index] = bytes[index] ^ key[index % stride];
        out[index] = ~out[index];
    }
    return OK;
}

s_status SenseControl::decBytes(uint8_t* bytes, int length, const uint8_t* key, int stride, uint8_t* out)
{
    if (!out)
        out = bytes;

    for (int index = 0; index < length; index++) {
        out[index] = bytes[index] ^ key[index % stride];
        out[index] = ~out[index];
    }
    return OK;
}

int SenseControl::encrypt(uint8_t* in, int len, uint8_t* res)
{
    s_msg* d = (s_msg*)in;

    // LOG_INFO("encrypt in data len %d mid %#x d->size_heb %#x,d->size_leb %#x\n", len, d->mid, d->size_heb, d->size_leb);
    // for (int i = 0; i < len - sizeof(s_msg); i++) {
    //     LOG_INFO("data[%d] %#x\n", i, d->data[i]);
    // }
    int ret = 0, enc_block = 0;
    if (!in || !res || len == 0) {
        LOG_ERROR("invalid params\n");
        return -1;
    }

    // for (enc_block = 0; enc_block < (len / ONE_ENC_BLOCK_SIZE); enc_block++) {
    //     aes_crypto(in + (enc_block * ONE_ENC_BLOCK_SIZE), ONE_ENC_BLOCK_SIZE, res + (enc_block * ONE_ENC_BLOCK_SIZE), (char*)mEncKey, AES_ENCRYPT);
    // }

    // if (len - (enc_block * ONE_ENC_BLOCK_SIZE)) {
    //     aes_crypto(in + (enc_block * ONE_ENC_BLOCK_SIZE), len % ONE_ENC_BLOCK_SIZE, res + (enc_block * ONE_ENC_BLOCK_SIZE), (char*)mEncKey, AES_ENCRYPT);
    // }
    ret = aes_crypto(in, len, res, (char*)mEncKey, AES_ENCRYPT);
    if (ret != 0) {
        LOG_ERROR("aes_crypto failed!\n");
        return -1;
    }
    // LOG_INFO("after AES_ENCRYPT\n");
    // for (int i = 0; i < len; i++) {
    //     LOG_INFO("data[%d] %#x\n", i, res[i]);
    // }
    return ret;
}

int SenseControl::decrypt(uint8_t* in, int len, uint8_t* res)
{
    s_msg* d = (s_msg*)in;
    // for (int i = 0; i < 16; i++) {
    //     LOG_INFO("mEncKey[%d] %#x\n", i, mEncKey[i]);
    // }
    // LOG_INFO("decrypt in data len %d mid %#x d->size_heb %#x,d->size_leb %#x\n", len, d->mid, d->size_heb, d->size_leb);
    // for (int i = 0; i < len - sizeof(s_msg); i++) {
    //     LOG_INFO("data[%d] %#x\n", i, d->data[i]);
    // }
    int ret = 0, enc_block = 0;
    if (!in || !res || len == 0) {
        LOG_ERROR("invalid params\n");
        return -1;
    }

    // for (enc_block = 0; enc_block < (len / ONE_ENC_BLOCK_SIZE); enc_block++) {
    //     aes_crypto(in + (enc_block * ONE_ENC_BLOCK_SIZE), ONE_ENC_BLOCK_SIZE, res + (enc_block * ONE_ENC_BLOCK_SIZE), (char*)mEncKey, AES_DECRYPT);
    // }

    // if (len - (enc_block * ONE_ENC_BLOCK_SIZE)) {
    //     aes_crypto(in + (enc_block * ONE_ENC_BLOCK_SIZE), len % ONE_ENC_BLOCK_SIZE, res + (enc_block * ONE_ENC_BLOCK_SIZE), (char*)mEncKey, AES_DECRYPT);
    // }
    ret = aes_crypto(in, len, res, (char*)mEncKey, AES_DECRYPT);
    if (ret != 0) {
        LOG_ERROR("aes_crypto failed!\n");
        return -1;
    }
    // LOG_INFO("after AES_DECRYPT\n");
    // for (int i = 0; i < len; i++) {
    //     LOG_INFO("data[%d] %#x\n", i, res[i]);
    // }
    return ret;
}
#endif