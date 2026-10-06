/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-14 22:05:20
 * @LastEditors: 洪照
 * @LastEditTime: 2025-08-30 23:36:31
 * @FilePath: /sense-koks/application/communication/src/updater.cpp
 */
#include "updater.h"
#include "message_v2.h"
#include "os_utils.h"
#include "partition_guard.h"
#include "rota_common.h"
#include "sys_logger.h"
#include "rota_update.h"

using namespace st::sense_koks;

Senselock* UpdaterHandler::sLock = nullptr;

static uint8_t rota_lock_getState()
{
    return UpdaterHandler::sLock->mState;
}

static void rota_lock_setState(uint8_t state)
{
    UpdaterHandler::sLock->mState = state;
}

static void rota_lock_reply(uint8_t mid, uint8_t result, void* data, uint16_t dataSize, s_type type)
{
    UpdaterHandler::sLock->reply(mid, result, data, dataSize, (s_type)type);
}

static void rota_lock_sendMessage(uint8_t mid, void* data, uint16_t dataSize, s_type type)
{
    UpdaterHandler::sLock->sendMessage(mid, data, dataSize, (s_type)type);
}

UpdaterHandler::UpdaterHandler()
{
    sLock = Senselock::getInstance();
    static RotaLockCallbak callback = {rota_lock_getState, rota_lock_setState, rota_lock_reply, rota_lock_sendMessage};
    rota_update_init(&callback);
}

UpdaterHandler* UpdaterHandler::getInstance()
{
    static UpdaterHandler instance;
    return &instance;
}

void UpdaterHandler::startOTA(s_type type)
{
    rota_enable_sfc1();
    rota_update_startOTA(type);
}

void UpdaterHandler::getOTAStatus(s_type type)
{
    rota_update_getOTAStatus(type);
}

void UpdaterHandler::handleMsgOTAHeader(s_msg* msg, s_type type)
{
    rota_update_handleMsgOTAHeader(msg, type);
}

void UpdaterHandler::OTAPacketTransmit(s_msg* msg, s_type type)
{
    rota_update_OTAPacketTransmit(msg, type);
}

void UpdaterHandler::stopOTA(s_type type)
{
    rota_update_stopOTA(type);
}
