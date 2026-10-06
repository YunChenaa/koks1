/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-14 22:05:26
 * @LastEditors: 洪照
 * @LastEditTime: 2025-08-12 17:34:25
 * @FilePath: /sense-koks/application/communication/include/updater.h
 */
#pragma once
#include "senselock.h"

class UpdaterHandler
{
    public:
    static UpdaterHandler* getInstance();
    void startOTA(s_type type = s_type::TYPE_UART);
    void getOTAStatus(s_type type = s_type::TYPE_UART);
    void handleMsgOTAHeader(s_msg* msg, s_type type = s_type::TYPE_UART);
    void stopOTA(s_type type = s_type::TYPE_UART);
    void OTAPacketTransmit(s_msg* msg, s_type type = s_type::TYPE_UART);

    public:
    static Senselock* sLock;

    private:
    UpdaterHandler();
};