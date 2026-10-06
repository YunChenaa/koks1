/**
 * @file OVD_openapi_lock.h
 * @brief 门锁品类相关业务接口定义
 * @author 中移杭州信息技术有限公司
 * @copyright 2021 OVD All rights reserved.
 */

#ifndef OVDOPENAPI_LOCK_H
#define OVDOPENAPI_LOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "OVD_define.h"


/**
 * @brief 门锁图片结构体
 *
 */
typedef struct ovd_lockimageinfo{
    OVD_char *buf;  ///< 图片数据buff
    OVD_int32 size; ///< 图片数据大小
}OvdLockImageInfo, ovd_lockimageinfo_t;


/**
 * @brief 门锁开锁类型映射表
 *
 */
typedef enum
{
    OVD_UNLOCKWAY_PWD=0,                ///< 密码开锁
    OVD_UNLOCKWAY_FP=1,                 ///< 指纹开锁
    OVD_UNLOCKWAY_RADIOCARD=2,          ///< 射频卡开锁
    OVD_UNLOCKWAY_APP=3,                ///< APP开锁
    OVD_UNLOCKWAY_KEY=4,                ///< 钥匙开锁
    OVD_UNLOCKWAY_SUPPERPWD=5,          ///< 超级密码
    OVD_UNLOCKWAY_ANTIFLOW_PWD=6,       ///< 防尾随密码(劫持)开锁
    OVD_UNLOCKWAY_ANTIFLOW_FP=7,        ///< 防尾随指纹(劫持)开锁
    OVD_UNLOCKWAY_INDOOR=8,             ///< 室内下拉把手打开锁

    OVD_UNLOCKWAY_TMPPWD=10,            ///< 临时密码开锁
    OVD_UNLOCKWAY_SUPPERSIM=11,         ///< 超级SIM卡开锁
    OVD_UNLOCKWAY_FACE=12,              ///< 人脸识别开锁
    OVD_UNLOCKWAY_TMPRADIOCARD=13,      ///< 临时视频卡开锁
    OVD_UNLOCKWAY_MANAGERFP=14,         ///< 管理指纹开锁
    OVD_UNLOCKWAY_MANAGERADIOCARD=15,   ///< 管理射频卡开锁
    OVD_UNLOCKWAY_GROUP=16,             ///< 组合开锁
    OVD_UNLOCKWAY_GROUP_FP_PWD=17,      ///< 指纹+密码组合开锁
    OVD_UNLOCKWAY_GROUP_FP_CARD=18,     ///< 指纹+门卡组合开锁
    OVD_UNLOCKWAY_GROUP_PWD_CARD=19,    ///< 密码+门卡组合开锁
    OVD_UNLOCKWAY_REMOTE=20,            ///< 通话远程开锁

    OVD_UNLOCKWAY_OTHER,                ///< 其他开锁方式

} ovd_unlockway_e;



/**
 * @brief 门锁异常事件类型映射表
 *
 */
typedef enum
{
    OVD_UNLOCKEXCEPTION_NULL=0,         ///< 无异常
    OVD_UNLOCKEXCEPTION_BROKEN=1,       ///< 撬门
    OVD_UNLOCKEXCEPTION_FP=2,           ///< 指纹输入异常
    OVD_UNLOCKEXCEPTION_PWD=3,          ///< 密码输入异常
    OVD_UNLOCKEXCEPTION_OPEN=4,         ///< 虚掩
    OVD_UNLOCKEXCEPTION_HIJACK=5,       ///< 劫持
    OVD_UNLOCKEXCEPTION_CARD=6,         ///< 卡解锁异常
    OVD_UNLOCKEXCEPTION_FACTORYRESET=7, ///< 恢复出厂设置
    OVD_UNLOCKEXCEPTION_LATCH=8,        ///< 门栓阻塞告警
    OVD_UNLOCKEXCEPTION_FROZEN=9,       ///< 门锁冻结报警
    OVD_UNLOCKEXCEPTION_TMPPWD_OPEN=10, ///< 临时密码开锁
    OVD_UNLOCKEXCEPTION_INVALID=11,     ///< 非法用户(非法尝试开门)
    OVD_UNLOCKEXCEPTION_KEEPOPEN=12,    ///< 常开
    OVD_UNLOCKEXCEPTION_LOCKFAILED=13,  ///< 门未锁好报警
    OVD_UNLOCKEXCEPTION_INDOORUNLOCK=14,///< 室内开锁报警

    OVD_UNLOCKEXCEPTION_KEEPOPEN_IGNORE=16,///< 取消常开报警
    OVD_UNLOCKEXCEPTION_PEOPLE_IN=17,   ///< 门口来人
    OVD_UNLOCKEXCEPTION_PEOPLE_OUT=18,  ///< 门口人离开
    OVD_UNLOCKEXCEPTION_RING=19,        ///< 门铃提醒
    OVD_UNLOCKEXCEPTION_INDOORLOCK=20,  ///< 已反锁
    OVD_UNLOCKEXCEPTION_INDOORLOCKCLOSE=21,///< 反锁解除
    OVD_UNLOCKEXCEPTION_FIRE=22,        ///< 火宅提醒
    OVD_UNLOCKEXCEPTION_TMPPWD=23,      ///< 临时密码开锁异常
    OVD_UNLOCKEXCEPTION_NETWORK=24,     ///< 门锁网络异常
    OVD_UNLOCKEXCEPTION_SUPPERSIM=25,   ///< 超级SIM卡解锁异常
    OVD_UNLOCKEXCEPTION_FACE=26,        ///< 人脸识别解锁异常
    OVD_UNLOCKEXCEPTION_MULTI=27,       ///< 多方式解锁异常

    OVD_UNLOCKEXCEPTION_OTHER,      ///< 其他异常

} ovd_unlockexception_e;



/**
 * @brief 门锁业务事件类型映射表
 *
 */
typedef enum
{
    OVD_LOCKEVENT_SUPPERSIM_ADD=0,      ///< 新增超级SIM卡
    OVD_LOCKEVENT_SUPPERSIM_MOD=1,      ///< 修改超级SIM卡
    OVD_LOCKEVENT_SUPPERSIM_DEL=2,      ///< 删除超级SIM卡
    OVD_LOCKEVENT_FP_ADD=3,             ///< 新增指纹
    OVD_LOCKEVENT_FP_MOD=4,             ///< 修改指纹
    OVD_LOCKEVENT_FP_DEL=5,             ///< 删除指纹
    OVD_LOCKEVENT_CARD_ADD=6,           ///< 新增卡片
    OVD_LOCKEVENT_CARD_MOD=7,           ///< 修改卡片
    OVD_LOCKEVENT_CARD_DEL=8,           ///< 删除卡片
    OVD_LOCKEVENT_PWD_ADD=9,            ///< 新增密码
    OVD_LOCKEVENT_PWD_MOD=10,           ///< 修改密码
    OVD_LOCKEVENT_PWD_DEL=11,           ///< 删除密码
    OVD_LOCKEVENT_TMPPWD_ADD=12,        ///< 新增临时密码
    OVD_LOCKEVENT_TMPPWD_MOD=13,        ///< 修改临时密码
    OVD_LOCKEVENT_TMPPWD_DEL=14,        ///< 删除临时密码
    OVD_LOCKEVENT_FACE_ADD=15,          ///< 新增人脸
    OVD_LOCKEVENT_FACE_DEL=16,          ///< 删除人脸
    OVD_LOCKEVENT_GROUP_PWD_FP_ENABLE=17,   ///< 已启用密码+指纹组合开锁模式
    OVD_LOCKEVENT_GROUP_PWD_FP_DISABLE=18,  ///< 已禁用密码+指纹组合开锁模式
    OVD_LOCKEVENT_GROUP_PWD_CARD_ENABLE=19, ///< 已启用密码+门卡组合开锁模式
    OVD_LOCKEVENT_GROUP_PWD_CARD_DISABLE=20,///< 已禁用密码+门卡组合开锁模式
    OVD_LOCKEVENT_GROUP_FP_CARD_ENABLE=21,  ///< 已启用指纹+门卡组合开锁模式
    OVD_LOCKEVENT_GROUP_FP_CARD_DISABLE=22, ///< 已禁用指纹+门卡组合开锁模式
    OVD_LOCKEVENT_GROUP_ENABLE=23,          ///< 已启用组合开锁模式
    OVD_LOCKEVENT_GROUP_DISABLE=24,         ///< 已禁用组合开锁模式

    OVD_LOCKEVENT_OTHER,                    ///< 其他业务

} ovd_lockevent_e;

/**
 * @brief 门锁电池索引映射表
 *
 */
typedef enum
{
    OVD_LOCKBATTERY_MAIN=0,             ///< 主电池
    OVD_LOCKBATTERY_BACK=1,             ///< 备份电池
    OVD_LOCKBATTERY_OTHER,              ///< 其他

}ovd_lockbatteryindex_e;


/**
 * @brief 门锁品类上报开锁记录
 *
 * @param channel [in] 通道号，对于门锁单摄像头品类，该字段传0
 * @param type [in] 开锁方式，开锁方式数值参考开锁类型映射表
 * @param timestamp [in] 开锁时间 开锁的绝对时间戳，从Epoch（1970年一月一日凌晨零点零分零秒）开始计算的毫秒数
 * @param imageinfo [in] 开锁抓拍图片信息
 * @param desc [in] 辅助描述性字符串
 *
 * @return 0代表成功，非0代表失败
 */
OVD_EXPORT OVD_int32 ovd_unlock_way_report(OVD_int32 channel,  ovd_unlockway_e type, OVD_uint64 timestamp,OvdLockImageInfo *imageinfo,OVD_char * desc);




/**
 * @brief 门锁异常事件上报
 *
 * @param channel [in]  通道号，对于门锁单摄像头品类，该字段传0
 * @param type [in]   门锁异常事件类型，门锁异常事件类型数值参考 门锁异常事件类型映射表
 * @param timestamp [in]  开锁时间 开锁的绝对时间戳，从Epoch（1970年一月一日凌晨零点零分零秒）开始计算的毫秒数
 * @param imageinfo [in]     开锁抓拍图片信息
 * @param desc [in]  辅助描述性字符串
 * @return 0代表成功，非0代表失败
 */
OVD_EXPORT OVD_int32 ovd_unlock_exception_report(OVD_int32 channel,  ovd_unlockexception_e type, OVD_uint64 timestamp,OvdLockImageInfo *imageinfo, OVD_char * desc);




/**
 *  @brief 门锁品类上报业务事件
 *  @param channel [in]  通道号，对于门锁单摄像头品类，该字段传0
 *  @param type [in]   业务事件类型， 业务事件数值参考门锁业务事件类型映射表
 *  @param timestamp [in]  开锁时间 开锁的绝对时间戳，从Epoch（1970年一月一日凌晨零点零分零秒）开始计算的毫秒数
 *  @param  imageinfo [in]     开锁抓拍图片信息
 *  @param desc [in]  辅助描述性字符串
 *  @return  0代表成功，非0代表失败
*/
OVD_EXPORT OVD_int32 ovd_lock_event_report(OVD_int32 channel,  ovd_lockevent_e type, OVD_uint64 timestamp,OvdLockImageInfo *imageinfo, OVD_char * desc);



/**
 * @brief 门锁电量上报
 *
 * @param channel [in]  通道号，对于门锁单摄像头品类，该字段传0
 * @param index [in]   门锁电池序列号，0代表主电池，1代表备份电池
 * @param battery [in] 电池上报的电量
 * @return 0代表成功，非0代表失败
 */
OVD_EXPORT OVD_int32 ovd_lock_battery_report(OVD_int32 channel,  ovd_lockbatteryindex_e index, OVD_int32 battery);


/**
 * @brief 门锁电压上报
 *
 * @param channel [in]  通道号，对于门锁单摄像头品类，该字段传0
 * @param index [in]   门锁电池序列号，0代表主电池，1代表备份电池
 * @param voltagestatus [in] 电池上报的电压状态，布尔型，false代表低压状态，true代表正常状态
 * @return 0代表成功，非0代表失败
 */
OVD_EXPORT OVD_int32 ovd_lock_voltage_report(OVD_int32 channel,  ovd_lockbatteryindex_e index, OVD_bool voltagestatus);


#ifdef __cplusplus
}
#endif

#endif //OVDOPENAPI_OPENAPI_H

