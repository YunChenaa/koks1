/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-13 22:45:56
 * @LastEditors: 洪照
 * @LastEditTime: 2025-06-30 16:47:48
 * @FilePath: /sense-koks/application/communication/include/sense_control.h
 */
#pragma once
#include "koks_project_def.h"
#include "message_l.h"
#include "sys_logger.h"
#include <atomic>
#include <chrono>
#include <list>
#include <message_v2.h>
#include <mutex>
#include <stdio.h>
#include <string>
#include <thread>
#ifdef KOKS_PROJECT_M33
#define MAX_RECEIVE_DATA_SIZE (32 * 1024 + 112)
#else
#define MAX_RECEIVE_DATA_SIZE 4112
#endif
#define MAX_REPLY_DATA_SIZE 260
#define MAX_NOTE_DATA_SIZE 260
#define MAX_SEND_IMAGE_SIZE 4000

class HostListener
{
    public:
    virtual void onMsg(s_msg& msg) = 0;
};

class HostProxy
{
    public:
    virtual void setListener(HostListener* listener) = 0;
    virtual void sendMsg(s_msg_note_data* smsg, uint16_t note_data_size, s_type type = s_type::TYPE_UART) = 0;
    virtual void sendReply(s_msg_reply_data* rdata, uint16_t size, s_type type = s_type::TYPE_UART) = 0;
    virtual void sendImg(unsigned char* image, uint16_t image_size) = 0;
};

#define ENC_KEY_HEADER "ENCKEY:"

enum EncMode { ST_ENCMODE_DEFAULT = 0, ST_ENCMODE_AES = 1, ST_ENCMODE_SMPL = 2 };

typedef enum { OK, UNKNOWN_ERROR, INVALID_PARAM, INVALID_STATE, NO_MEMORY, EXCEED_LIMITATION, NO_CAMDEV, INVALID_LICENSE, ALL_FILE_LOCKED, FILE_LOCKED } s_status;

class SenseControl : public HostProxy
{
    public:
    static SenseControl* getInstance();
    ~SenseControl();

    static const int ENC_KEY_SIZE = 16;

    s_status init();
    s_status deinit();

    // XOR calculation
    uint8_t checksum(uint8_t* buff, int size);

    void process_data(unsigned char* buf, uint16_t len);
    void cdc_process_data(unsigned char* buf, uint16_t len);
    bool isFirstCmdReceived() const { return first_cmd_received; }
    void sendReadyMessage();
    // HostProxy
    void sense_uart_send(uint8_t* send_buffer, uint32_t send_len);
    void sense_cdc_send(uint8_t* send_buffer, uint32_t send_len);
    void send(uint8_t reply_id, void* data, uint16_t data_size, int max_buf_size, s_type type = s_type::TYPE_UART);
    void sendNoEncrypt(uint8_t reply_id, void* data, uint16_t data_size, int max_buf_size, s_type type = s_type::TYPE_UART);
    virtual void sendMsg(s_msg_note_data* smsg, uint16_t note_data_size, s_type type = s_type::TYPE_UART);
    virtual void sendReply(s_msg_reply_data* rmsg, uint16_t reply_data_size, s_type type = s_type::TYPE_UART);
    virtual void sendReplyNoEncrypt(s_msg_reply_data* rmsg, uint16_t reply_data_size, s_type type = s_type::TYPE_UART);
    virtual void sendImg(unsigned char* image, uint16_t image_size);
    virtual void setListener(HostListener* listener)
    {
        mListener = listener;
    }

    // this MUST BE called in side the listener callback funtion
    void setEncryptionMode(EncMode mode)
    {
        mEncryptMode = mode;
    }
    EncMode getEncryptionMode()
    {
        return mEncryptMode;
    }

    HostListener* mListener;
    EncMode mEncryptMode;
    char mEncKey[ENC_KEY_SIZE + 1];
    char mDebugEncKey[ENC_KEY_SIZE + 1];

    bool checkKeyNumberLegal(uint8_t* key_number);
    s_status setKey(uint8_t* seed, int size);
    s_status ReConfigureKeyFromSavedMd5();
    s_status setKeyNumber(uint8_t* key_number, uint8_t key_type, bool enforce);
    s_status checkRlsKey();
    /* TODO: encrytion work should be encapsulated to a virtual interface */
    // AES encrytion
    int encrypt(uint8_t* in, int len, uint8_t* res);
    int decrypt(uint8_t* in, int len, uint8_t* res);
    // sensetime simple encode/decode
    s_status encBytes(uint8_t* bytes, int length, const uint8_t* key, int stride, uint8_t* out = NULL); // Encryption bytes
    s_status decBytes(uint8_t* bytes, int length, const uint8_t* key, int stride, uint8_t* out = NULL); // Decryption bytes

    // config uart baudrate
    s_status config_uart(uint32_t baudrate);
    std::list<void*> mUartMSGQueue;
    bool has_rls_key;
    std::atomic<bool> has_dbg_key{ false };
    std::atomic<bool> first_cmd_received{ false };

    private:
    SenseControl();
    bool hasEncKey(uint8_t key_type, uint8_t* key_number);

    s_status getKeyNumber(uint8_t* key_number);
    std::mutex send_mutex;
    uint8_t encrypt_xor[4] = { 0x41, 0x94, 0x31, 0x42 };
    uint8_t savedSeedMd5[32];
    uint8_t rls_key_number[ENC_KEY_SIZE];
};
