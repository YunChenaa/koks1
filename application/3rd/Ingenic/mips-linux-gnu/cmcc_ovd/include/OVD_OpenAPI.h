/**
 * @file OVD_OpenAPI.h
 * @brief   OVD SDK 接口定义
 * @author 中移杭州信息技术有限公司
 * @copyright 2021 OVD All rights reserved.
 *
 */




#ifndef OVDOPENAPI_OPENAPI_H
#define OVDOPENAPI_OPENAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "OVD_define.h"
#include "menuconfig.h"


typedef struct
{

    /**
     * @brief [回调]获取设备信息
     * @ingroup 设备信息查询
     * @details OVC获取OVD设备信息时调用，返回相应的 @ref OVDDeviceInfo 信息。若当前设备为有线设备，则wifi相关字段置空即可。
     * @param [out] deviceInfo        需要设备返回的信息，详细见结构体 @ref OVDDeviceInfo
     * @return
     * -  0:  成功
     * - < 0:  失败
     */
    OVD_int32(*OVD_GetOVDDeviceInfo)(OUT OVDDeviceInfo *deviceInfo);

    /**
     * @brief 【回调】获取设备网络接口名称
     * @details 设备初始化时调用，返回所有网络接口名称:例如eth0, wlan0，注：不包括回环接口
     * @param [out] 
     * @return
     * - >  0: 网卡个数
     * - <= 0: 失败
     */
    OVD_int32(*OVD_GetInterfacesCallback)(OUT char ifaces[][IFNAMSIZ], IN OVD_int32 max_count);

    /**
     * @brief [回调]获取设备Gps信息
     * @ingroup 设备信息查询
     * @details OVC获取行车记录仪坐标信息时调用，厂商返回响应坐标信息
     * @param [out] gpsinfo    需要返回的坐标信息，详见ovd_gps_info_t
     * @return
     * -  0:  成功
     * - < 0:  失败
     */
    OVD_int32 (*OVD_GetGpsInfo)(OUT ovd_gps_info_t *gpsinfo);


    /**
    * @brief [回调] 获取远程配置信息
    * @ingroup 设备配置
    * @details OVC获取OVD远程配置信息时调用
    *
    * @param [out] output_ovdconfig  字符串，厂商负责分配内存malloc,由设备sdk负责释放
    * @param [out] output_size       字符串长度
    *
    * @return
    * -  0:  成功
    * - < 0:  失败
    */
    OVD_int32(*OVD_GetOVDConfigureInfo)(OUT OVD_char **output_ovdconfig, OUT OVD_int32 *output_size);

    /**
    * @brief [回调]设置远程配置信息
    * @ingroup 设备配置
    * @details OVC下发修改参数，OVD启用相应配置的参数。没有携带的信息，设备保持原参数配置。
    *
    * @param [in]  in_ovdconfig      需要设置的信息，json格式信息
    * @param [out]  errMsg           必要信息返回,内存由sdk分配，无信息回NULL
	* @param [in]  errMsg_len            errMsg长度
    *
    * @return
    * - 0:  成功
    * - < 0:  失败
    */
    OVD_int32(*OVC_SetOVDConfigureInfo)(IN OVD_char *in_ovdconfig, OUT OVD_char *errMsg,IN OVD_int32 errMsg_len);

    /**
    * @brief [回调]获取单个视频通道远程配置信息
    * @ingroup 设备配置 多通道
    * @details OVC获取OVD某个通道远程配置信息时调用
    *
    * @param [in]  channel 视频通道号
    * @param [out] output_ovdconfig  字符串，厂商负责分配内存malloc,由设备sdk负责释放
    * @param [out] output_size       字符串长度
    *
    * @return
    * -  0:  成功
    * - < 0:  失败
    */
    OVD_int32(*OVD_GetOVDChannelConfigureInfo)(IN OVD_int32 channel, OUT OVD_char **output_ovdconfig, OUT OVD_int32 *output_size);






    /**
    * @brief [回调]连接服务器结果回调
    * @ingroup 安防业务状态通知
    *
    * @details SDK与OVC之间的连接状态发生变化时（连接上/连接断开），通过此接口通知OVD。
    *               OVD根据状态的通知，决定相应异步处理。
    *
    * @note 当前信令平台鉴权是在OVD与OVC连接成功后才开始鉴权，故会出现先OVD_OVCConnectStatus 反馈连接成功，后因鉴权失败导致反馈连接失败。
    *
    * @param [in] connectStatus      0:连接成功;     -1:连接失败;     1:重连中
    */
    OVD_void(*OVD_OVCConnectStatus)(const IN OVD_int32 connectStatus);





    /**
    * @brief [回调]重启通道(NVR设备)
    * @ingroup 多通道 设备控制
    *
    * @details 当用户通过APP远程控制channel重启时，调用此接口;若OVD不支持单channel重启，那么就重启设备
    *
    * @param [in] channel     需要重启的channel
    *
    * @return
    * - = 0:  成功
    * - < 0:  失败
    */

    OVD_int32(*OVD_ReBootChannel)(const IN OVD_int32 channel);

    /**
      * @brief [回调]重启设备
      * @ingroup 设备控制
      * @details sdk通知厂商重启
      * @param [in]: reason sdk同步的重启原因，目前主要包含自动维护重启，软看门狗重启，远程重启
      * @note 厂商收到此回调了，务必同步执行重启动作，不能跟其他业务进行耦合
      *
      * @return
      * -  0:  成功
      * - < 0:  失败
      */
    OVD_int32 (* OVD_ReBootDevice)(const IN ovd_reboot_reason_e reason);

    /**
    * @brief [回调]恢复默认设置
    * @ingroup 设备控制
    * @details 当OVC决定设备恢复出厂配置时，调用此接口。
    *               设备收到该请求后，应该将所有配置恢复到出厂状态（包括wifi配置），但不能断开当前网络连接，并返回成功应答。
    *               OVC稍后会再下发一个重启指令将设备重启，默认配置生效。
    *
    * @return
    * - = 0:  成功
    * - < 0:  失败
    */
    OVD_int32(*OVD_ResetConfiguration)();





    /**
    * @brief 【回调】升级指令与升级数据
    * @ingroup OTA
    * @details 部署在OTA服务器上的固件包是使用杭研的工具firmware_encrypt加密过的，且
    *          带有128字节的头部数据，SDK回调 \c OVD_FirmwareBinUpgrade 接口送入固件
    *          侧的数据已进行了如下操作：
    * - 校验128字节头部数据
    * - 去除头部128字节数据
    * - 对固件解密
    *
    * @note    执行./firmware_encrypt -h 可查看加密工具的帮助信息。
    *
    * @note    如果升级是有可能变砖的，需要在收到 \c OVD_CMD_UPGRADE_PKG_INFO 指令
    *          时保存升级的相关信息，主要包括固件url地址和加密算法类型、加密密钥，然后
    *          在uboot下或者是在小系统下完成第二次升级。此次升级需要厂商自行从url通过
    *          http下载固件。下载之后需执行如下操作：
    *          - 去除128字节头部
    *          - 对固件解密，之前已经保存过了解密需要的密钥和算法类型
    *          - 在uboot或者小系统中完成升级
    *
    * @param [in] channel_id                通道id，一般入参为0
    * @param [in] cmd                       升级命令字
    * @param [in] data                      不同命令字对应的数据指针
    * @param [in] size                      数据大小
    *
    * @return
    * -  0:  成功
    * - < 0:  失败
    */

    OVD_int32 (*OVD_FirmwareBinUpgrade)(OVD_int32 channel_id, OVD_Upgrade_cmd_e cmd, void* data, size_t size, void* user_data);

    /**
     * @brief [回调]升级结果确认
     * @ingroup OTA
     * @param [in] channel_id   通道id，一般入参为0
     * @param [in] info         升级结果确认信息
     *
     * @note 在升级过程中，SDK不会调用该接口查询状态。每次设备启动时，SDK在初始化阶段会调用该接口查询固件升级状态。
     * - 如果设备是正常启动，未进行过升级，则 \c info 里面的状态信息填 \c OVD_UPGRADE_CONFIRM_NORMAL
     * - 如果设备是升级后的重启，则 \c info 里面的状态信息填 \c OVD_UPGRADE_CONFIRM_SUCCUSS 或者 \c OVD_UPGRADE_CONFIRM_FAIL
     *
     */
    OVD_int32 (*OVD_FirmwareUpgradeConfirmCb)(OVD_int32 channel_id, OVD_UpgradeConfirmInfo_t *info);



    /**
	* @brief [回调]云台控制
	* @ingroup 设备控制 云台
	* @details APP端控制OVD进行转动时，调用此接口
	*
	* @param [in] channel			通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
	* @param [in] ptzcmd			    控制命令,详细可见枚举类型OVCPTZControlCmd
	* @param [in] ptzvalue			参考下图
	*
    * | op | 操作 | value |
    * | --- | --- | --- |
    * | up | 上 | 可选，整数，速度，0-100，0最慢，100最快，默认50 |
    * | down | 下 | 同上 |
    * | left | 左 | 同上 |
    * | right | 右 | 同上 |
    * | upleft | 左上 | 同上 |
    * | upright | 右上 | 同上 |
    * | downleft | 左下 | 同上 |
    * | downright | 右下 | 同上 |
    * | zoomin | 拉近 | 同上 |
    * | zoomout | 拉远 | 同上 |
    * | stop | 停止 | 可选，整数，但数值没有意义 |
    * | goto_preset | 跳转预置位 | 预置位ID， 0-255 |
    * | set_preset | 设置预置位 | 同上 |
    * | clear_preset | 清除预置位 | 同上 |
    * | up_step | 单步上 | 可选，整数，单步步长，0-100，0最小，100最大，默认0 |
    * | down_step | 单步下 | 同上 |
    * | left_step | 单步左 | 同上 |
    * | right_step | 单步右 | 同上 |
    * | upleft_step | 单步左上 | 同上 |
    * | upright_step | 单步右上 | 同上 |
    * | downleft_step | 单步左下 | 同上 |
    * | downright_step | 单步右下 | 同上 |
    * | zoomin_step | 单步拉近 | 同上 |
    * | zoomout_step | 单步拉远 | 同上 |
    * | focusdown    | 焦距调小 | 同上 |
    * | focusup      | 焦距调大 | 同上 |
    * | zoom_reset   | 恢复默认倍率 | 同上 |
    * | focusdown_setp | 焦距单步调小 | 同上 |
    * | focusup_step   | 焦距单步调大 | 同上 |
    *
	* @return
	* -  OVD_PTZ_OK                : 成功
    * -  OVD_PTZ_CTRL_FAIL         : PTZ控制失败
    *
    * -  OVD_PTZ_PRESET_SET_EXSIT  : PTZ预置位已存在
	* -  OVD_PTZ_PRESET_SET_FAIL   : PTZ预置位设置失败
    *
    * -  OVD_PTZ_PRESET_CLEAR_FAIL : PTZ预置位清除失败
    */
    ovd_ptz_cmd_code (*OVD_PTZCmd)(IN OVD_int32 channel,IN OVCPTZControlCmd ptzcmd,IN OVD_int32 ptzvalue);

    /**
	* @brief [回调]获取预置位
	* @ingroup 云台 设备信息查询
	* @details OVC下发命令，OVD返回前期配置的预置点信息
	*
	* @param [in] channel			    通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
	* @param [out] presetList			返回设备当前已设置的预置点列表，非数组索引,sdk已分配内存
	* @param [in] array_malloc_num		sdk内部分配的预置点数组大小
	* @param [out] count			        预置点实际个数
	* @return
 	* - = 0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_GetPresetList)(IN OVD_int32 channel,OUT OVD_int32 *presetList,IN OVD_int32 array_malloc_num, OUT OVD_int32 *count);




    /**
	* @brief [回调]EBO机器人指令控制
	* @ingroup 机器人业务
	* @details APP端控制OVD进行转动时，调用此接口
	*
	* @param [in] ebocmd			类型，详情见OVCEBOControlCmd
	* @param [in] data			数据
    *
	* @return
	* - 0:  成功
	* - < 0:  失败
    */
	OVD_int32 (*OVD_EBOCmd)(IN OVCEBOControlCmd ebocmd,IN void* data);

	/**
	* @brief [回调]机器人控制
	* @ingroup 机器人业务
	* @details APP端控制机器人，调用此接口
	*
	* @param [in] robotcmd		控制命令,详细可见结构体ovd_robot_content_t
	* @param [out] data			    参考下图
	*
    * | op                  | 操作        |
    * | ------------------  | ------------|
    * | OVC_ROBOT_OWL       | 猫头鹰机器人 |
    *
	* @return
	* - = 0:  成功
	* - < 0:  失败
    */
	OVD_int32 (*OVD_ROBOTCmd)(IN ovd_robot_content_t* robotcmd, OUT void* data);

    /**
    * @brief [回调]homibot数据下发回调
    * @ingroup 机器人业务
    * @param [in] cmd    命令字
    * @param [in] config 下发数据内容,json格式
    * @param [in] size   下发数据字节数
    *
    * @return
    * - = 0:  成功
    * - < 0:  失败
    */
    OVD_int32(*OVD_homibot_set_config)(IN OVD_char *cmd, IN OVD_char *config, IN OVD_int32 size);

    /**
    * @brief [回调]homibot数据上报回调
    * @ingroup 机器人业务
    * @param [in] cmd    命令字
    * @param [in] buffer 返回上报数据
    * @param [in] size   buffer的空间大小
    *
    * @return
    * - < 0:  失败
    * - = 0:  上报数据为空
    * - > 0   上报数据大小
    */
    OVD_int32(*OVD_homibot_get_config)(IN OVD_char *cmd, IN OVD_char *buffer, IN OVD_int32 size);


    /**
     * @brief [回调]强制I帧
     * @ingroup SDK流媒体
     * @details SDK在收到上传视频等命令时，为了服务器/客户端能够最短时间内播放视频，调用此接口在100ms内设备底层编码出一个I帧喂入
     * @param [in] channel	 通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
     * @param [in] code_stream 多码流,code_stream为多码流的索引，主码流为OVD_HIGH_STREAM，子码流为OVD_LOW_STREAM，无多码流情况下默认为OVD_HIGH_STREAM
     * @return
 	 * -  0:  成功
 	 * - < 0:  失败
     */

    OVD_int32 (*OVD_ForceIFrame)(IN OVD_int32 channel, IN OVDCodeStream code_stream);

    /**
	* @brief [回调]封面截图
	* @ingroup 设备信息查询
	* @details OVD截取channel上的当前图片，反馈截图内容及信息。若截图大小大于maxImageSize，则返回-2并在OVDImageInfo.size中带回所需要图片大小，之后会再调用一次。
	*
	* @param [in] channel			   通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
	* @param [out] ImageInfo			   截取的图片信息
	*
	* @return
    *  - 0：成功
    *  - -1： 截图失败
    *  - -2：  maxImageSize空间不足，并在OVDImageInfo.size中带回所需要图片大小
    */
    OVD_int32 (*OVD_Snapshot)(IN OVD_int32 channel,OUT OVDImageInfo *imageInfo, OUT OVD_int32 maxImageSize);

    /**
     * @brief sdk告警逻辑状态回调通知厂商侧
    * @ingroup 告警 安防业务状态通知
    * @details
    * @param [in] type      告警逻辑状态通知类型
    * @param [in] buf		 在该告警逻辑状态下的数据
    * @param [in] len		 在该告警逻辑状态下数据长度
    * @return
    * - = 0:  成功
    * - < 0:  失败
    */
    int (*OVD_AlarmNotifyCallback_t)(IN OVD_AlarmNotifyType_e type, IN const void* buf, IN unsigned int len);


    /**
     * @brief  安防业务推流状态通知
    * @ingroup 安防业务状态通知
    * @details 若厂商关注sdk内部推流状态，则需要注册该函数，sdk将通过该函数反馈推流状态（主要涉及直播，云存业务）
    * @param [in] type       推流业务通知类型
    * @param [in] channel    通道号
    * @return
    * - = 0:  成功
    * - < 0:  失败
    */
    int (*OVD_StreamNotifyCallback_t)(IN OVD_StreamNotifyType_e type, IN const unsigned int channel);

    /**
     * @brief  平台登录失败原因通知
     * @ingroup 安防业务状态通知
     * @details 若厂商关注sdk内部信令上线业务失败的原因（非网络环境原因)，则需要注册该函数，sdk将通过该函数反馈信令上线失败原因
    *
    * @details
    * @param [in] err_code      失败错误码
    * @param [in] err_str       失败原因描述
    * @return
    * - = 0:  成功
    * - < 0:  失败
    */
    int (*OVD_NotifyLoginFail)(IN int err_code, IN char *err_str);




    /**
    * @brief [回调]OVC通知OVD播放音乐/音频时调用,暂不启用
    * @ingroup 设备控制
    * @details OVD去url下载音乐内容，并在channel的音箱上播放
    * @deprecated 音乐播放业务暂未上线
    * @param [in]channel:      通道号
    * @param [in]url:          歌曲/音频下载的url
    * @param [in]repeat:       播放次数，默认为1次，有效值为1~10
    * @param [in]volume        播放音量，默认用当前音量，有效值为0~100
    *
    * @return
    * - 0： 成功
    * - -1： 失败
    */
    OVD_int32 (*OVD_SetAudioOutPlay)(IN OVD_int32 channel,IN const OVD_char *url,IN int repeat, IN int volume);

    /**
    * @brief [回调]OVC通知OVD进行童谣业务控制
    * @ingroup 设备控制
    * @details 控制设备童谣业务，如播放，暂停，下一首，设置为安抚音乐等，当前用于宝宝看护产品
    * @param [in]channel:      通道号
    * @param [in]album_id:     专辑id
    * @param [in]music_id:     歌曲id
    * @param [in]music_name:   歌曲名
    * @param [in]url:          歌曲url
    * @param [in]ctrl_type:    控制类型（ 0：从头开始播放  1：暂停   2.继续播放  3：下一首歌  4：上一首歌曲 5：将当前歌曲设置为最后一首，6：设置为安抚音乐 其他：备用）
    *
    * @return
    * - 0： 成功
    * - -1： 失败
    */
   OVD_int32 (*OVD_CtrlLullby)(IN OVD_int32 channel,IN const OVD_char *album_id,IN const OVD_char* music_id, IN const OVD_char* music_name, IN const OVD_char* url, IN int ctrl_type);

    /**
	* @brief [回调]从ovd设备中获取时间戳函数，精确度毫秒
	* @ingroup 设备信息查询
	* @param [out] out_time
	*
	* @return
 	* - 0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_gettime)(OUT OVD_uint64 *out_time);

    /**
	* @brief [回调]设置设备时间
	* @ingroup 设备控制
	* @details 首次连接或者重新调度时设置设备时间
	*
	* @param [in] input_time			   设置的时间戳，单位为毫秒
	* @param [in] tolerance_value	   设置的容忍值，单位为秒,即当前系统时间戳和预设置的时间相差小于容忍值，则不需要设置系统时间
	*
	* @return
 	* - 0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_settime)(const IN OVD_uint64 input_time, const IN OVD_int32 tolerance_value);


    /**
	* @brief [回调]设备状态获取
	* @ingroup 设备信息查询  设备状态变更
	* @details 回调时机为服务启动初期，sdk回调厂商接口
	*
	* @param [out] out_simpleovd			 sdk已分配内存 channel_count不能少于1
	*
	* @return
 	* -  0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_getsimpleovdinfo)(OUT SimpleOVDinfo *out_simpleovdinfo);


    /**
	* @brief [回调] 卡状态查询
	* @ingroup 设备信息查询
	* @param [out] out_state			   必填，整数；sd卡状态，0：正常，1：未插卡，2：卡异常,3格式化中
	* @param [out] out_total			   必填，整数, sd卡总容量，单位为MB
	* @param [out] out_free			   必填, 整数, sd卡剩余容量，单位为MB
	*
	* @return
 	* - 0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_GetDiskInfo)(OUT OVD_int32 *out_state, OUT OVD_int32 *out_total, OUT OVD_int32 *out_free);

	/**
	* @brief [回调]OVD格式化SD卡
	* @ingroup 设备控制
	* @details OVC需要格式化SD卡时调用，厂商需异步处理
	*
	* @return
 	* - 0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_SetSDCardFormat)();






    /**
     * @brief 日志上传
     * @ingroup 日志
     * @details OVC可以通过该命令控制OVD上传相关本地日志，供调试分析使用。
	 *               OVD收到该指令后，通过异步方式将指定时间范围日志上传到相应的URL。
	 *               若指定范围的日志已经滚动删除，则使用尽力原则上传剩余的日志。
	 *               若指定范围没有任何日志记录，应上传一个长度为0的日志文件。
	 *               日志文件通过PUT方法上传。
	* @param [out] trans_id		   必填，字符串；标记此日志上传的任务ID
	* @param [out] start			   必填，字符串；日志记录开始时间，格式yyyy-MM-ddTHH:mm:ss，例子：2016-12-05T02:15:32
	* @param [out] end			   必填，字符串；日志记录结束时间，格式yyyy-MM-ddTHH:mm:ss，例子：2016-12-06T02:15:32
	* @param [out] url			   必填，字符串；日志上传的URL，设备通过PUT方法上传相应的日志文件
	*
	* @return
 	* - 0:  成功
 	* - < 0:  失败
     */


    OVD_int32 (*OVD_LogUploadAsync)(OUT OVD_char *trans_id, OUT OVD_char *start, OUT OVD_char* end,OUT OVD_char* url);



    /**
     * @addtogroup 扩展
     * @brief
     *
     * @{
     */

    /**
    * @brief [回调]平台扩展指令
    * @ingroup 安防业务状态通知
    * @details 第三方平台指令可通过此回调下发,2022/8/15,此功能废弃，使用固话原来对接方式
    * @deprecated 该业务已下线
    * @note 目前支持和家智话平台长连接ccrtc
    *
    * @param [in] method                信令里的method字段
    * @param [in] in_data               信令里带的method_param字段
    * @param [out] out_response         第3方回调返回的字符串内容
    * @param [in] in_outresponse_len    预分配的内存字符串长度，目前预分配1024(包括\0)，厂商不能超过1000长度
    *
    * @return
    * - 0：成功
	* - -1： 通用设备错误
	* - -2： 标识mehod字段 厂商不支持
	* - -3：参数不合法或无效
	*/
    OVD_int32(*OVD_extension_callback)(IN char *method, IN char *in_data, OUT char *out_response, IN OVD_int32 in_outresponse_len,
                                       OUT OVD_int32 *out_outresponse_len);


    /**
	* @brief [回调]消除告警接口
	* @ingroup 告警
	* @details 消除指定类型告警，若type为0，则消除所有当前告警
	*
	* @param [in] alarmtype	指定告警类型
    *
    * @return
 	* -  0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_StopAlarm)(OVDAlarmType alarmtype);

    /**
     * @brief AI DIY配置项回调
     * @ingroup AIDIY端云协同
     * @details SDK在上线AI平台后，会将配置项下发给设备，如标签上报间隔（即OVD_AIDIY_Report调用频率）
     *
     * @param [in] config  AI DIY配置项
     */
    OVD_void (*OVD_AIDIY_ConfigCallback)(IN OVD_AIDIY_Config config);

    /**
	* @brief [回调]低功耗设备休眠控制
    * @ingroup 低功耗
    *
	* @note 1. OVD在没活动的情况下自动关闭主芯片电路，并启动网络接口（卡/板）的休眠功能。
    * 2. 网络接口（卡/板）与OVH（休眠服务器）的休眠服务IP地址/端口建立TCP长连接。
    * 3. 网络接口（卡/板）每60秒向OVH休眠服务发送一个心跳报文。
    * 4. 若收到OVH发出的唤醒报文，则停止网络接口（卡/板）的休眠功能，启动主芯片电路，正常接入OVC其余服务。
	*
	* @details OVC可以通过该方法阻止设备（及指定通道）在指定时间内休眠，或者唤醒设备（及指定通道）当前所有休眠的部件。
	*               OVD收到此指令后，应该在指定的过期时间内，保证设备（及指定通道）能够完全正常上电工作。收到此命令expired秒后，由设备自行决定是否进入休眠状态。
	*
	* @param [in] channel			   需要保持不休眠的channel
	* @param [in] notAllowHibernate	   是否允许进入休眠状态，0：允许进入休眠，进入休眠时间根据expired参数确定；1：不允许进入休眠，此种情况下，不管expired设置为多少，设备都要保持上电状态，不休眠，直至收到下一个休眠指令
	* @param [in] expired			   从收到命令起，到expired的时间内，保持不休眠
	* @param [in] reason			       设置不休眠的原因，详见枚举值定义OVDHibernateReason
	*
	* @return
 	* -  0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_KeepAwakenUtilExpired)(IN OVD_int32 channel, IN OVD_int32 notAllowHibernate, IN OVD_int32 expired, IN OVDHibernateReason reason);






	/**
	* @brief [回调] (p2p接口)OVD根据入参的配置，查询SD卡中的文件列表，并返回文件列表信息
	* @ingroup 卡回放
    * @deprecated p2p卡回放业务也废弃
	* @details OVC查看OVD上SD卡的录像文件信息列表时，调用此接口
	*
	* @param [in] channelmask1			   需要查询的通道mask，对应低位号的通道         （弃用)
	* @param [in] channelmask2			   需要查询的通道mask，对应高位号的通道 弃用
	* @param [in] recordTypemask			   文件类型（第0位：视频文件 第1位：告警文件） 弃用
	* @param [in] startStamp			   	   录像查询的起始时间戳(单位为秒)
	* @param [in] endStamp			       录像查询的结束时间戳(单位为秒)
	* @param [in] page			           查询的页码
	* @param [in] numInPage			       页的条数
	* @param [out] fileInPage			   录像文件列表信息，详细可见结构体描述OVDRecordFileListPerPage
	*
	* @return
 	* -  0:  成功
 	* - < 0:  失败
 	*
 	* @note 假设查询的录像文件数有200个，numInPage=10，则最大页码Page为20。若传进的参数numInPage=10，page=2，则fileInPage应该返回第10个到第20个录像的信息;若传进的参数numInPage=10，page=21，则fileInPage返回空录像信息(录像个数为0)
	*/
    OVD_int32(*OVD_QueryRecordPage)(IN OVD_uint32 channelmask1, IN OVD_uint32 channelmask2, IN OVD_uint32 recordType, IN OVD_uint64 StartStamp,
                                    IN OVD_uint64 EndStamp, IN OVD_int32 Page, IN OVD_int32 numInPage, OUT OVDRecordFileListPerPage *FilePage);

	/**
	* @brief [回调](p2p接口)OVD根据入参的文件名称，读取SD卡上相应文件的音视频数据参数信息及录像时长，反馈到出参
	* @ingroup 卡回放
	* @details OVC根据查询到的SD卡文件列表信息，决定查看一个文件详细信息，调用此接口
	* @deprecated p2p业务已废弃
	* @param [in] channel			   通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
	* @param [in] recordname			   录像文件名称
	* @param [out] videoInfo			   视频信息,详细可见结构体描述OVDVideoDataFormat
	* @param [out] audioInfo			   音频信息,详细可见结构体描述OVDAudioDataFormat
	* @param [out] fileTotalTime		   该录像的时长(秒)
	*
	* @return
 	* - = 0:  成功
 	* - < 0:  失败
	*/
    OVD_int32(*OVD_OpenRecordFile)(IN OVD_int32 channel, IN OVD_char *recordname, OUT OVDVideoDataFormat *videoInfo,
                                   OUT OVDAudioDataFormat *audioInfo, OUT OVD_int32 *fileTotalTime);

	/**
	* @brief [回调](p2p接口)OVD根据控制信息，控制文件的内容是否通过OVD_SendRecordAVContent上传文件内容
	*
	* @details OVC根据查询到的SD卡文件列表信息，控制相应音视频文件的播放、停止、暂停等信息
	* @ingroup 卡回放
    * @deprecated p2p业务已废弃
	* @param [in] channel			    通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
	* @param [in] controlType			播放控制，详细可见枚举类型OVDCONTROLTYPE
	* @param [in] value			        额外值，目前只有视频拖动时会用到，代表要跳至的视频时间戳(ms)
	*
	* @return
 	* -  0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_RecordCotrol)(IN OVD_int32 channel,IN OVDCONTROLTYPE controlType,IN OVD_int32 value);

    /**
     * @brief [回调]卡录像查询
     * @details OVC可以通过该指令后去获取指定时间区间的卡录像记录
     * @ingroup 卡回放
     * @note RecordSearch 和 RecordSee k非强关联，两者互不干扰
	 * @param [in] channel			   通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
	 * @param [in] starttime		   录像查询的起始时间戳(单位为秒,绝对时间戳)，当天0点
	 * @param [in] endtime			   录像查询的结束时间戳(单位为秒,绝对时间戳)，当前时间
	 * @param [in] page			       查询的页面索引，若一次查询区间不够，sdk会回调第二次
	 * @param [in] numinpage		   该页面最大的区间数量
	 * @param [out] fileInPage		   厂商回填区间信息
     * @return
     * -  0:  成功
     * - < 0:  失败
     */

    OVD_int32(*OVD_DMEAPI_callback_RecordSearch)(IN OVD_int32 channel, IN OVD_uint64 starttime, IN OVD_uint64 endtime, IN OVD_int32 page,
            IN OVD_int32 numInPage, OUT OVD_DMERecordFileListPerPage *fileinpage);

    /**
	* @brief [回调]卡录像打开RecordOpen
	* @ingroup 卡回放
	* @details 通知厂商打开一条某一channel下的卡录像会话。
	*
	* @param [in] channel			 通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
	*
	* @return
 	* - 成功: 返回卡录像会话厂商侧的上下文（context）标识，ctx，会话句柄非单个文件句柄
 	* - 失败: NULL
	*/
    OVD_void * (*OVD_DMEAPI_callback_RecordOpen)(IN OVD_int32 channel);

	/**
	* @brief [回调]卡录像播放RecordSeek
	* @ingroup 卡回放
	* @details 该调用后，sdk下一次读取音视频数据帧时，厂商需从相近时间点的I帧（优先向前）开始返回数据。若对应时间点没有录像数据则返回错误。
	*
	* @param [in] ctx			    厂商卡录像会话上下文标识，为RecordOpen返回的ctx
	* @param [in] timestamp			绝对时间戳，seek的绝对时间点,单位为秒
	*
	* @return
 	* -  0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_DMEAPI_callback_RecordSeek)(IN OVD_void* ctx, IN OVD_int64 timestamp);

    /**
	* @brief [回调]卡录像数据读取RecordReadFrame
	* @ingroup 卡回放
	* @details pframe_info 结构体内存由sdk分配, 由sdk 负责释放。
    *               pframe_info里的frame_buf 由厂商分配，也由厂商释放;建议厂商在下次read或者会话结束释放内存。
	*
	* @param [in] ctx			   厂商卡录像会话上下文标识
	* @param [in] pframe_info：	   帧数据信息
	*
	* @return
 	* - OVD_RET_COMMON_ERROR：通用错误码
    * - OVD_RET_SUCCESS：成功
    * - OVD_RET_READ_FRAME_RETRY：读帧重试错误码
    * - OVD_RET_READ_FRAME_EOF：卡录像读取完毕，厂商反馈EOF
	*
   	* @note 若收到通用错误，则sdk会调用销毁卡录像会话资源机制
    */
    OVD_int32 (*OVD_DMEAPI_callback_RecordReadFrame)(IN OVD_void* ctx, IN OVD_FrameInfo *pframe_info);

    /**
	* @brief [回调]卡录像数据倍速读取RecordSpeedReadFrame
	* @ingroup 卡回放
	* @details pframe_info 结构体内存由sdk分配, 由sdk 负责释放。
	*               speed 告诉厂商SDK以什么速度读取卡录像文件，需要厂商支持倍速播放功能，该倍速功能由能力集OVD_CARD_PLAYBACK_SPEED_MASK_xx控制
    *               pframe_info里的frame_buf 由厂商分配，也由厂商释放;建议厂商在下次read或者会话结束释放内存。
    *               该接口是OVD_DMEAPI_callback_RecordReadFrame的升级接口，建议使用该接口进行对接
	*
	* @param [in] ctx			   厂商卡录像会话上下文标识
	* @param [in] speed			   取帧速度
	* @param [in] pframe_info：	   帧数据信息
	*
	* @return
 	* - OVD_RET_COMMON_ERROR:    通用错误码
    * - OVD_RET_SUCCESS:         成功
    * - OVD_RET_READ_FRAME_RETRY: 读帧重试错误码
    * - OVD_RET_READ_FRAME_EOF=202：卡录像读取完毕，厂商反馈EOF
	*
   	* @note 若收到通用错误，则sdk会调用销毁卡录像会话资源机制
    */
    OVD_int32 (*OVD_DMEAPI_callback_RecordSpeedReadFrame)(IN OVD_void* ctx, IN ovd_playback_speed_times_e speed, IN OVD_FrameInfo *pframe_info);

    /**
	* @brief [回调]卡录像会话关闭RecordClose
	* @ingroup 卡回放
	* @param [in] ctx		 厂商卡录像会话上下文
	*
	* @return
 	* - 0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_DMEAPI_callback_RecordClose)(IN OVD_void *ctx);



/**
	* @brief [回调](p2p接口)OVD打开OVD对讲功能
	* @ingroup 对讲
    * @deprecated p2p业务也被废弃
	* @details APP端发起对讲时，调用此接口打开OVD对讲
	*
	* @param [in] channel			   通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
	* @param [in] andioFormat		   对讲音频数据的格式信息，见结构体定义OVDAudioOutDataFormat
	*
	* @return
 	* -  0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_AudioPlayStart)(IN OVD_int32 channel, IN OVDAudioOutDataFormat andioFormat);

    /**
	* @brief [回调](p2p接口)OVD接收数据，并在音箱播放出来
	* @ingroup 对讲
	* @details OVC把对讲的音频内容，传输给OVD时调用
	*
	* @param [in] channel		通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
	* @param [in] buf			音频数据指针
	* @param [in] size			音频数据大小
	*
	* @return
 	* -  0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_AudioPlayProGress)(IN OVD_int32 channel,IN OVD_uchar* buf, IN OVD_int32 size);

    /**
	* @brief [回调](p2p接口)OVD关闭对讲功能
	* @ingroup 对讲
    * @deprecated p2p业务也被废弃
	* @details APP关闭对讲时通知OVD
	*
	* @param [in] channel			通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
	*
	* @return
 	* -  0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_AudioPlayStop)(IN OVD_int32 channel);



    /**
	* @brief [回调](p2p接口)切换视频清晰度，已弃用
	* @ingroup 设备控制
    * @deprecated p2p业务也被废弃
	* @details OVD根据配置切换到相应的视频清晰度，并返回切换后的视频参数
	*
	* @param [in] channel			通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
	* @param [in] quality			要设置的清晰度，详细参考枚举值OVDEncodeQuality
	* @param [out] vedioInfo			清晰度调整后的视频参数，详细参考结构体OVDVideoDataFormat
    *
    * @return
 	* -  0:  成功
 	* - < 0:  失败
	*/
    OVD_int32 (*OVD_VedioSwitchQuality)(IN OVD_int32 channel, IN OVDEncodeQuality quality, OUT OVDVideoDataFormat *vedioInfo);

	/**
	* @brief [回调](p2p接口)获取SD卡信息，已弃用
	* @ingroup 设备信息查询
    * @deprecated p2p业务已被废弃
	* @details OVD查询本地SD卡相关信息（OVDSDInfo定义的内容），反馈信息
	*
	* @param [out] sdInfo			存储卡信息,详细可见结构体描述OVDSDInfo
    *
    * @return
 	* -  0:  成功
 	* - < 0:  失败
	*/

	OVD_int32 (*OVD_GetSDInfo)(OUT OVDSDInfo *sdInfo);



    /**
    * @brief [回调]获取设备信息，目前用于软探针
    * @ingroup 设备信息查询 软探针
    * @param [in] info_e         获取信息类型,详细可见结构体描述ovd_probe_devrunning_info_e,注意ping和traceroute，不支持回调返回101 ;服务器url不可达，回调返回102
    * @param [out] data          相应信息类型返回数据,使用可见demo,字符串类型长度为64
    *
    * @return
    * -  0:  成功
    * - < 0:  失败
    */
    OVD_int32(*OVD_GetDevRunningInfo)(IN ovd_probe_devrunning_info_e info_e, OUT void *data);

    /**
	 * @brief [回调]异常运行状态通知设备
     * @ingroup 安防业务状态通知
	 * @param [in] status  状态类型，详细见ovd_probe_exception_type_e
	 * @param [in] channel_id 通道号
	 * @param [in] data      相应信息类型返回数据
	 * @param [in] user_data 用户数据指针
	 * @return 返回函数执行结果
	 * - OVD_LOGIC_EC_SUCCESS：成功
	 * - 其他值：失败
	 */
	OVD_int32(*ovd_running_status_cb)(IN ovd_probe_exception_type_e status, IN OVD_int32 channel_id, IN void* data, IN void* user_data);




     /**
     * @brief [回调]srt对讲打开通道
     * @ingroup 对讲
     * @param [in] channel  通道号
     * @param [in] st  音频参数结构体，详情见OVDAudioDataFormat
     * @return
     * - 0:  成功
     * - < 0:  失败
     */
    OVD_int32 (*OVD_AudioOpenDev)(IN OVD_int32 channel,IN OVDAudioDataFormat* st);

     /**
     * @brief [回调]srt对讲关闭通道
     * @ingroup 对讲
     * @param [in] channel  通道号
     * @return
     * -  0:  成功
     * - < 0:  失败
     */
    OVD_int32 (*OVD_AudioCloseDev)(IN OVD_int32 channel);

     /**
     * @brief [回调]srt对讲音频播放
     * @ingroup 对讲
     * @param [in] channel  通道号
     * @param [in] timestamp  时间戳(ms)
     * @param [in] flags  标志位
     * @param [in] buff  音频数据
     * @param [in] len   音频数据长度
     * @return
     * -  0:  成功
     * - < 0:  失败
     */
    OVD_int32 (*OVD_AudioPlayData)(IN OVD_int32 channel,IN OVD_int64 timestamp, IN int flags, IN unsigned char* buff, IN int len);


    /**
    * @brief [回调]设置云存套餐信息
    * @ingroup 安防业务状态通知
    * @param [in] cloudStatusType    云存状态，开启或者关闭
    * @param [in] info_e             云存套餐信息，详细可见结构体描述OVD_SetCloudInfo_e
    *
    * @return
    * -  0:  成功
    * - < 0:  失败
    */
    OVD_int32(*OVD_SetCloudStatus)(IN OVD_bool cloudStatusType, IN OVD_SetCloudInfo_e info_e);



    /**
    * @brief [回调] AOV功能开关回调
    * @ingroup AOV
    * @param[in] channel	 通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
    * @param[in] status_e    设置AOV功能是否开启，详细可见结构体描述 enum OvdAovStatus
    *
    * @return
    * -  0:  成功
    * - < 0:  失败
    */
    OVD_int32(*OVD_set_aov_switch)(IN OVD_int32 channel, IN OVD_AOV_STATUS_e status_e);


    /**
    * @brief [回调]设备直播状态信息回调
    * @ingroup AOV
    * @param[in] channel	 通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
    * @param[in] info_e      某通道下的直播状态信息，详细可见结构体描述 OVD_SetLiveInfo_e
    *
    * @return
    * -  0:  成功
    * - < 0:  失败
    */
    OVD_int32(*OVD_set_live_status)(IN OVD_int32 channel, IN OVD_SetLiveInfo_e info_e);

    /**
     * @brief    绑定/解绑通知
     * @ingroup 安防业务状态通知
     * @details  当设备绑定或者解绑时，将状态同步给厂商
     * @note 有部分厂商需求需要感知绑定状态，若无需求可不关注这个回调
     * @return
     * -  0:  成功
     * - < 0:  失败
     */
    OVD_int32(*OVD_bind_status_cb)(IN OVD_binding_status_e status_e);

    /**
     * @brief   SDK通知厂商日志上传，厂商将需要上传的日志放到指定目录，SDK负责打包上传SDK日志、厂商指定的日志
     * @ingroup 日志
     * @details 同步接口，厂商负责传入关键日志，比如flash保存的日志、系统日志等
     *
     * @param [in] log_path，SDK指定的日志目录
     *
     * @return void
     */
    OVD_int32 (*OVD_LogNotifyCb)(const IN OVD_char *log_path);


    /**
     * @brief AI智能语音对讲业务相关设备状态通知回调
     * @ingroup 智能语音交互
     *
     * @param [in] OVD_speechStatusNotifyType_e       状态通知类型
     * @param [in] session_id       会话id
     * @param [in] buf				在该状态通知类型下的数据
     * @param [in] len				在该状态通知类型下数据长度
     * @return
     * -  0:  成功
     * - < 0:  失败
    */
    OVD_int32 (*OVD_SpeechStatusNotify_cb)(IN OVDSpeechStatusNotifyType_e type,IN  char* session_id, IN const void* buf, IN unsigned int len);

    /**
     * @brief 语音对讲业务云台相关控制指令回调
     * @ingroup 智能语音交互
     * @param [in] type             云台控制指令类型
     * @param [in] session_id		会话id
     * @param [in] text				指令可能伴有音频，则该字段传递音频的文本
     * @param [in] sync_ts_ms		下发指令的绝对时间戳（毫秒）
     * @return
     * -  0:  成功
     * * - < 0:  失败
    */
    OVD_int32 (*OVD_SpeechPTZCtrl_cb)(IN OVDSpeechPTZControlType_e type,IN const char* session_id, IN const char* text, IN unsigned long long sync_ts_ms);

    /**
     * @brief 语音对讲设备相关控制指令回调
     * @ingroup 智能语音交互
     * @param [in] type             设备控制指令类型
     * @param [in] session_id		会话id
     * @param [in] value            操作的具体值，字符串，“true”代表打开，“false"代表关闭
     * @param [in] text				指令可能伴有音频，则该字段传递音频的文本
     * @param [in] sync_ts_ms		下发指令的绝对时间戳（毫秒）
     * @return
     * -  0:  成功
     * * - < 0:  失败
    */
    OVD_int32 (*OVD_SpeechDevCtrl_cb)(IN OVDSpeechDEVControlType_e type,IN const char* session_id,IN const char* value, IN const char* text, IN unsigned long long sync_ts_ms);

    /**
     * @brief 语音对讲第3方媒体资源播放
     * @ingroup 智能语音交互
     * @param [in] session_id		会话id
     * @param [in] sync_ts_ms		下发指令的绝对时间戳（毫秒）
     * @param [in] url              第3方媒体资源url
     * @return
     * -  0:  成功
     * - < 0:  失败
    */
    OVD_int32 (*OVD_SpeechMediaDownloadAndPlay_cb)(IN const char* session_id,IN unsigned long long sync_ts_ms, IN const char * url);

    /**
     * @brief 语音对讲固话呼叫控制指令回调
     * @ingroup 智能语音交互
     * @param [in] type             固话呼叫指令类型
     * @param [in] session_id		会话id
     * @param [in] phonenum         目标电话号码
     * @param [in] nickname			目标昵称
     * @param [in] sync_ts_ms		下发指令的绝对时间戳（毫秒）
     * @return
     * - 0:  成功
     * - < 0:  失败
    */
    OVD_int32 (*OVD_SpeechMakeCall_cb)(IN OVDSpeechCallType_e type,IN const char* session_id, IN const char* phonenum, IN const char* nickname, IN unsigned long long  sync_ts_ms);


    /**
    * @brief [回调]wifi感知状态查询
    * @ingroup 设备信息查询
    * @param [in] wifi_status     wifi感知状态，字符串，uneffective:未生效，learning:学习中，failure:学习失败，effective:已生效
    * @param [in] linkage_status  摄像头联动感知状态，字符串，uneffective:未生效，effective:已生效
    * @param [in] desc  WIFI运动感知状态描述
    * @return
    * - 0:  成功
    * - < 0:  失败
    */
    OVD_int32 (*OVD_WifiSensingStatusQuery_cb)(IN char* wifi_status, IN char* linkage_status, IN char* desc);


#ifdef CONFIG_USING_CLOUD_AI


    /**
     * @brief [回调]云化AI控制和设置命令
     * @ingroup 动态算法
     * @param [in] cmd 命令字
     * @param [in] data 命令对应的数据指针，指针类型和命令字类型有关
     *
     */
    OVD_int32 (*OVD_CloudAiCtrl)(enum OVD_CLOUD_AI_CtrlCmd cmd, void *data);
#endif


#ifdef CONFIG_WHITE_BOX
    /**
     * @brief [回调]白盒摄像头设置license授权信息接口
     * @ingroup 白盒摄像头
     * @param [in] license 授权信息字符串
     * @param [in] signature 授权信息签名字符串
     *
     */
    OVD_int32 (*OVD_SetLicense)(const char *license, const char *signature);

    /**
     * @brief 【回调】白盒摄像头获取日志服务器url接口
     * @param [in] url 日志服务器域名
     *
     */
    OVD_int32 (*OVD_LogServerUrl_cb)(const char *url);
#endif

    /**
     * @brief [回调]手动枪球联动
     * @details 用户长按枪机画面，触发手动枪球联动，球机需转动到对应位置
     * @ingroup 设备控制 枪球联动
     * @param [in] pan  0-10000，画面位置横坐标
     * @param [in] tilt 0-10000，画面位置纵坐标
     * @return
     * -  0:  成功
     * - < 0:  失败
     */
    OVD_int32 (*OVD_LinkageTrackingManual)(IN OVD_int32 pan, IN OVD_int32 tilt);

    /**
     * @brief [回调]联动矫正
     * @ingroup 设备控制 枪球联动
     * @details 用户触发联动矫正时，厂商需进行枪球联动矫正，须为不阻塞接口
     * @return
     * - 0:  成功
     * - < 0:  失败
     */
    OVD_int32 (*OVD_LinkageTrackingFix)(void);

    /**
     * @brief [回调]联动矫正进度查询
     * @details App轮询联动矫正进度，厂商需返回当前联动矫正进度
     * @ingroup 设备信息查询 枪球联动
     * @param [out] progress 联动矫正进度，0-100
     * @return
     * - 0:  成功
     * - < 0:  失败
     */
    OVD_int32 (*OVD_LinkageTrackingFixQuery)(OUT OVD_int32 *progress);
    
    /**
     * @brief [回调]3D光传感器环境采集
     * @details 通知厂商进行环境采集操作，该接口为阻塞接口
     * @ingroup 3D光传感器
     * @param [in] channel	通道号
     * @return
     * - 0:  成功
     * - < 0:  失败
     * - OVD_RET_SUCCESS：成功
     * - OVD_RPC_3DSENSOR_IS_COLLECTING：正在环境采集
     * - OVD_RPC_3DSENSOR_NO_DETECT：未开启检测功能
     * - OVD_RPC_3DSENSOR_RECOGNIZE_PLANES_FAIL：识别平面失败
     * - OVD_RPC_3DSENSOR_COLLECT_GROUND_FAIL：收集地面系数和检测区域失败
     * - OVD_RPC_3DSENSOR_RECOGNIZE_BED_FAIL：识别床错误
     * - OVD_RPC_3DSENSOR_SAVE_FAIL：保存背景环境失败
     */
    OVD_int32 (*OVD_EnvCollection)(IN OVD_int32 channel);

} OVD_CallBackFunList;





/**
 * @brief   能力集初始化
 * @ingroup SDK 主要功能
 * @details 将入参capinfo进行默认值初始化。其中设置了默认值的参数有 have_sd:1;
 *          video_quality:"sd,hd"; video_formats_supportlists:"h264"
 *
 * @note    能力集修改后需要变更固件版本号才可生效，能力集详情参考doc目录里cap.txt文件
 *
 * @param [in] capinfo	厂商设备能力集
 *
 * @return
 * -  0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_CapInit(IN OVDCapInfo *capinfo) attribute_deprecated;

/**
 * @brief SDK初始化
 * @ingroup SDK主要功能
 * @deprecated 建议使用OVD_Initv2接口替代
 * @param [in] capinfo		     设备能力集
 * @param [in] clientParam       设备连接服务器信息
 * @param [in] logParam          日志配置信息
 * @param [in] reboot_info       重启信息
 * @param [in] callBackFunList   响应服务器的回调函数
 * @param [in] jsonParam         json格式化后的字符串，提供动态的参数配置。 \n
 * 	json格式信息参考如下：
 *
 * \code
 *	{
 *		"snapshotSize":<可选，整数： 单位为B，缩略图ovd sdk预分配的内存大小，若厂商不设置，则默认为500KB>
 *		"buffDuration":<可选，整数： 单位为秒，设备从上电到连接平台时，需要sdk缓存的音视频内容的时长，若不设置，则为0秒，主要针对于低功耗唤醒设备，正常非休眠设备，不必启用>
 *		"debug_ts":<可选，布尔型，调试开关，若开启，则设备sdk日志输出传入音视频每一帧的时间戳>
 *
 *      "cseg_maxcount":<可选，整型， 指定云存模块队列缓存最大切片数量，默认值为3个，默认值由平台指令下发>
 *      "snddropdelay":<可选，整型，单位为毫秒，指定底层流媒体srt连接最大缓存时延，默认值为10000 毫秒>
 *   }
 * \endcode
 *
 * @return
 * 	- \c OVD_RET_SUCCESS:      0  初始化成功
 * 	- \c OVD_RET_BADPARAMETER: 1  入参校验失败
 * 	- \c OVD_RET_COMMON_ERROR: -1  通用错误，初始化失败
 */
OVD_EXPORT OVD_int32 OVD_Init(IN OVDCapInfo *capinfo, IN OVDClientParam *clientParam, IN OVDLogParam *logParam, IN ovd_reboot_param_t *reboot_info,
                           IN OVD_CallBackFunList *callBackFunList, IN OVD_char *jsonParam) attribute_deprecated;


/**
 * @brief 获取SDK默认能力集
 * @ingroup SDK主要功能
 * @details 在对SDK进行初始化之前，先使用该接口获取默认能力集，用户可以根据实际情况对默
 *          认能力集按类别进行修改，然后再调用 @c OVD_InitV2 接口对SDK进行初始化，初
 *          始化时传入刚才修改好的能力集。
 *
 * @note 不同的设备品类获取到的默认能力集是不同的
 *
 * @param [in] type	设备品类
 * @param [in] param	设备相关描述
 * @param [out] caps 获取到的SDK默认能力集
 *
 * @return
 * - 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_CapGetV2(IN ovd_device_type_e type, IN OVD_cap_params_t *param, OUT OVDCapInfoV2_t *caps);

/**
 * @brief SDK初始化
 * @ingroup SDK主要功能
 * @param [in] capinfo		     设备能力集，使用 \c OVD_CapGetV2 接口获取，得到的
 *                               是默认能力集，可直接对其修改
 * @param [in] clientParam       设备连接服务器信息
 * @param [in] logParam          日志配置信息
 * @param [in] reboot_info       重启信息
 * @param [in] callBackFunList   响应服务器的回调函数
 * @param [in] jsonParam         json格式化后的字符串，提供动态的参数配置。 \n
 * 	json格式信息参考如下：
 *  \code
 *	{
 *		"snapshotSize":<可选，整数： 单位为B，缩略图ovd sdk预分配的内存大小，若厂商不设置，则默认为500KB>
 *		"buffDuration":<可选，整数： 单位为秒，设备从上电到连接平台时，需要sdk缓存的音视频内容的时长，若不设置，则为0秒，主要针对于低功耗唤醒设备，正常非休眠设备，不必启用>
 *		"debug_ts":<可选，布尔型，调试开关，若开启，则设备sdk日志输出传入音视频每一帧的时间戳>
 *
 *      "cseg_maxcount":<可选，整型， 指定云存模块队列缓存最大切片数量，默认值为3个，默认值由平台指令下发>
 *      "snddropdelay":<可选，整型，单位为毫秒，指定底层流媒体srt连接最大缓存时延，默认值为10000 毫秒>
 *   }
 *   \endcode
 *
 * @return
 * 	- \c OVD_RET_SUCCESS:      0  初始化成功
 * 	- \c OVD_RET_BADPARAMETER: 1  入参校验失败
 * 	- \c OVD_RET_COMMON_ERROR: -1  通用错误，初始化失败
 */
OVD_EXPORT OVD_int32 OVD_InitV2(IN OVDCapInfoV2_t *capinfo, IN OVDClientParam *clientParam, IN OVDLogParam *logParam, IN ovd_reboot_param_t *reboot_info,
                           IN OVD_CallBackFunList *callBackFunList, IN OVD_char *jsonParam);

/**
* @brief sdk反初始化接口
* @ingroup SDK主要功能
* @deprecated 该接口暂无场景建议使用
* @return
*  - \c OVD_RET_SUCCESS: 0 反初始化成功
*  - 其他：失败
*/
OVD_EXPORT OVD_int32 OVD_deint();

/**
 * @brief 开启服务
 * @ingroup SDK主要功能
 * @details 设备连接网络成功、音视频接口准备完毕等工作之后再调用.该接口为不阻塞接口
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_ServiceStart();

/**
 * @brief 关闭服务
 * @ingroup SDK主要功能
 * @details 重新配网等场景需停止服务时调用
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_ServiceStop();

/**
 * @brief    恢复出厂设置
 * @ingroup 设备状态变更
 * @details  当设备整机恢复出厂设置时，SDK保存在设备上的数据文件也要恢复出厂设置，由设
 *           备厂商调用该接口重置SDK数据文件
 *
 * @return
 * - 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_RestoreFactory(void);


/**
 * @brief   SDK内部日志级别调整
 * @ingroup 日志 调试
 * @param [in] value    日志等级，详见 \c OVDLogLevel
 * @note 若sdk日志级别由sdk控制，则正式版本不建议厂商调用该接口
 * @return
 * - 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_setloglevel(const IN OVDLogLevel value);

/**
 * @brief 日志上传结果
 * @ingroup 日志
 * @details 通知OVC日志文件上传完成
 *
 * @param [in] trans_id    必填，字符串；标记此日志上传的任务ID，与LogUploadAsync保持一致
 * @param [in] result      0 代表success / 1代表fail
 * @param [in] start       必填，字符串；日志记录实际开始时间，格式yyyy-MM-ddTHH:mm:ss，例子：2016-12-05T02:15:32
 * @param [in] end         必填，字符串；日志记录实际结束时间，格式yyyy-MM-ddTHH:mm:ss，例子：2016-12-06T02:15:32
 * @param [in] url         必填，字符串；日志上传的URL
 * @param [in] size        必填，长整性；上传日志文件的大小字节数
 *
 * @return
 * - 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_LogDone(IN OVD_char* trans_id, const IN OVD_int32 result, IN OVD_char* start, IN OVD_char *end, IN OVD_char *url,const IN OVD_uint64 size);






/**
 * @brief 声波初始化
 * @ingroup 安防配网
 * @details 设备检测到无网络配置信息，判断若使用声波配网，则调用此接口，传入声波参数
 *
 * @param [in] sampleRate		采样率
 * @param [in] bitWidth          位宽(8/16bit)
 *
 * @return
 * - 成功：返回声波句柄
 * - 失败：NULL
 */
OVD_EXPORT OVD_void* OVD_SoundWaveInit(const IN OVD_int32 sampleRate,const IN OVD_int32 bitWidth);

/**
 * @brief 声波配网回调通知识别开始
 * @ingroup 安防配网
*/
typedef OVD_void (*RecognizerStart)();

/**
 * @brief 声波配网回调通知识别结束
 * @ingroup 安防配网
*/
typedef OVD_void (*RecognizerFinish)(const IN OVD_int32 type, IN OVD_void *info, const IN OVD_int32 infoLen);

/**
 * @brief   开启声波配网
 * @ingroup 安防配网
 * @details 设备开始声波配网后，获取到音频文件，发送到SDK识别；声波识别模块识别出数据后，
 *          调用回调 \c end_cbfunc 返回给上层
 *
 * @param [in] recognizer    声波句柄
 * @param [in] starFunc    	识别开始回调函数；回调函数定义为 @ref RecognizerStart
 * @param [in] endFunc    	识别结束回调函数（此函数返回wifi信息）；回调函数定义为 @ref RecognizerFinish
 *
 * @note    因为受现实环境影响可能传入的音频信息有误，会导致识别出的声波数据异常，所以
 *          在end_cbfunc中如果识别到的数据不合预期，建议用户继续发送声波，厂商继续喂入
 *          声波数据，直到反馈的识别结果数据符合预期（即类型为OVD_XX_SSID_WIFI，即包
 *          含wifi配置和bindid)
 *
 * @return
 * - =  0 : 成功
 * - <  0 : 失败
 * - = 101: sdk未集成声波识别模块
 */
OVD_EXPORT OVD_int32 OVD_SoundWaveStart(IN OVD_void *recognizer,const IN RecognizerStart starFunc,const IN RecognizerFinish endFunc);

/**
 * @brief 声波配网传入声波数据
 * @ingroup 安防配网
 * @details 设备开始声波配网后，把采集到声波数据传入声波模块的句柄
 *
 * @param [in] recognizer    声波句柄
 * @param [in] data    		声波数据的首字节指针
 * @param [in] len    		声波数据长度
 *
 * @return
 * - 成功:  写入的数据长度
 * - 失败:  < 0
 */
OVD_EXPORT OVD_int32 OVD_SoundWaveWriteData(IN OVD_void *recognizer,const IN OVD_void *data, const IN OVD_uint64 len);

/**
 * @brief 厂商停止声波识别
 * @ingroup 安防配网
 * @details 识别完后，调用此接口,回收资源
 *
 * @param [in] recognizer    声波句柄
 *
 * @return
 * - =  0 : 成功
 * - <  0 : 失败
 * - = 101: sdk未集成声波识别模块
 */
OVD_EXPORT OVD_int32 OVD_SoundWaveStop(IN OVD_void *recognizer);


/**
 * @brief 二维码配网初始化
 * @ingroup 安防配网
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_QRString_init();

/**
 * @brief 二维码配网传入数据
 * @ingroup 安防配网
 * @details 设备读取字符串,返回解析结果
 *
 * @param [out] info     厂商分配内存的指针，识别成功后，识别出的数据会存入这个内存
 * @param [in] qr_str    具体一张二维码图片厂商解析出的字符串,包含'/0'
 * @param [in] qr_len    具体一张二维码图片厂商解析出的字符串长度,strlen的长度
 *
 * @return
 * -  OVD_RET_RPC_UNIMPLEMENT：  101  未集成字符串解析模块
 * -  OVD_RET_BADPARAMETER：       1  入参错误
 * -  OVD_RET_COMMON_ERROR：      -1  本次输入的字符串无法解析或者最终解析到数据有误，此时厂商可继续输入数据继续识别。
 * -  OVD_NETCONF_SUCCESS：        7  识别成功
 * -  OVD_NETCONF_LESS_DATA：      9  配网缺失数据，数据太长，二维码分页，需要厂商再次输入
 */
OVD_EXPORT OVD_int32 OVD_QRString(OUT OVDXXSSIDWiFiInfo *info, IN OVD_uchar *qr_str,const IN OVD_uint32 qr_len);

/**
 * @brief AP配网开始
 * @ingroup 安防配网
 * @details 厂商需在WiFi热点开启后再调用该函数，WiFi热点名称为CMQLINK-deviceType-XXXX，
 *          其中deviceType为设备在开发者门户申请的设备类型码，XXXX为设备产生的随机四位
 *          码，一般是MAC地址后缀
 *
 * @param [out] info       返回wifi信息,需厂家分配内存
 *
 * @return
 * - 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_ApConf_start(OUT OVDXXSSIDWiFiInfo *info);

/**
 * @brief 蓝牙配网数据发送数据回调
 * @ingroup 安防配网
 * @details 厂商需要实现此接口，在接收完配网信息后，SDK会调用此回调用于向和家亲发送配网状态等信息
 *
 * @param [in] chrs     发送数据的特征值类型枚举，详见 @ref ovd_ble_chrs_e
 * @param [in] data     发送的数据
 * @param [in] data_len 发送的数据长度
 *
 * @return
 * - 0:  成功
 * - < 0:  失败
 */
typedef OVD_int32 (*OVD_BLE_send_data_cb)(IN const ovd_ble_chrs_e chrs,IN const OVD_uchar *data, IN const OVD_int32 data_len);

/**
 * @brief 蓝牙配网初始化
 * @ingroup 安防配网
 * @details 厂商需要在开始蓝牙配网时调用此接口，告知SDK蓝牙信息，
 *          SDK会根据蓝牙信息生成蓝牙广播参数及GATT服务参数
 *
 * @param [in] ble_info             低功耗蓝牙信息
 * @param [out] ble_gatt            蓝牙广播参数及GATT服务参数
 * @param [in] send_data_cb         蓝牙数据发送回调
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */

OVD_EXPORT OVD_int32 OVD_BLE_init(IN const ovd_ble_info_t ble_info, OUT ovd_ble_gatt_t *ble_gatt, IN const OVD_BLE_send_data_cb send_data_cb);


/**
 * @brief 蓝牙配网接收数据解析
 * @ingroup 安防配网
 * @details 收到来自和家亲APP的蓝牙数据时调用（即通过characteristic_uuid_down_write发给设备的数据）
 *          厂商可循环调用该接口，直到解析出wifi信息
 *
 * @param [in]  data
 * @param [in]  data_len
 * @param [out] wifi_info
 * @return
 * - 0:  成功
 * - < 0:  失败
 */

OVD_EXPORT ovd_ble_parse_e OVD_BLE_recv_parser(IN const OVD_uchar *data, IN const OVD_int32 data_len, OUT OVDXXSSIDWiFiInfo *wifi_info);


/**
 * @brief   上报绑定信息phoneid/bindid
 * @ingroup 安防配网
 * @details 二维码配网/蓝牙配网/声波配网/AP配网获取到信息，即可调用此接口，sdk会保存bindid信息，无需
 *          等连接wifi成功后再调用。
 *
 * @param [in] bindid    绑定的app账号信息
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_DeviceBindInfo(IN OVD_char *bindid);

/**
 * @brief 设备上报配网类型及联网类型
 * @ingroup 安防配网
 * @details 设备上报网络连接类型。
 *          上报时机： 网络类型：设备上线后上报或者网络类型改变时上报。
 *                    配网模式类型：设备上线后上报或者配网模式改变时上报。
 *
 * @param [in] channel   必填，通道号，如果是摄像头产品，则通道号为0，如果是nvr等产品，channel则为前端摄像头的索引
 * @param [in] networkType   必填，设备当前网络连接类型，\n
 *                         - 1: 无线网络
 *                         - 2：有线网络
 *                         - 3：有线和无线都配置
 *                         - 4:4G供网
 *                         - 5:5G供网
 *                         - 6:4G供网和有线都配置
 *                         - 7:5G供网和有线都配置
 * @param [in] SSID   必填，设备WIFI名称，无线网络、有线和无线都配置时填入，有线网络时为空
 * @param [in] IpAddr   必填，设备ip地址
 * @param [in] DN_mode   必填，设备配网模式，见 OVD_DNM_e
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_NetworkConnection(IN OVD_int32 channel, IN OVD_int32 networkType, IN OVD_char* SSID, IN OVD_char* IpAddr, IN OVD_DNM_e DN_mode);





/**
 * @brief 音视频准备（音视频格式输入）
 * @ingroup SDK流媒体
 * @param [in] channel     通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
 * @param [in] code_stream 码流（主码流为OVD_HIGH_STREAM，子码流为OVD_LOW_STREAM,不支持多码流默认为OVD_HIGH_STREAM）
 * @param [in] videoinfo   视频信息（视频格式要求是h264和h265）
 * @param [in] audeoinfo   音频信息,若无音频则为空（音频格式要求是AAC_with_ADTS）
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_AVPushStart(const IN OVD_int32 channel, const IN OVDCodeStream code_stream, IN OVDVideoDataFormat *videoinfo, IN OVDAudioDataFormat *audeoinfo);


/**
 * @brief 音视频编码参数修改
 * @ingroup SDK流媒体
 * @details 已经启动音视频传送后，若设备的音视频参数修改，则调用此接口通知sdk变动的参数
 *          注意分辨率切换不调用此接口，视频编码切换可以调用此接口此接口内部会有停止推流、
 *          改变参数、重新推流机制，故云存会有缺失
 *
 * @param [in] channel     通道号,如果是摄像头产品，则通道号为0，如果是nvr等产品，channel则为前端摄像头的索引
 * @param [in] code_stream 码流索引,如果支持主子码流，\c OVD_HIGH_STREAM 为主码流，
 *                         \c OVD_LOW_STREAM 为子码流，不支持多码流，默认为
 *                         \c OVD_HIGH_STREAM
 * @param [in] videoinfo   视频信息,详见结构体 \ref OVDVideoDataFormat ,若无视频则为空，若无修改也需要携带原来参数
 * @param [in] audeoinfo   视频信息,详见结构体 \ref OVDAudioDataFormat ,若无音频则为空，若无修改也需要携带原来参数
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_AVParamModify(const IN OVD_int32 channel, const IN OVDCodeStream code_stream, IN OVDVideoDataFormat *videoinfo, IN OVDAudioDataFormat *audeoinfo);

/**
 * @brief   音视频内容推送接口
 * @ingroup SDK流媒体
 * @details 设备向SDK推动音视频内容
 *
 * @note    debug_ts置为1，日志将输入每帧参数打印
 *
 * @param [in] channel       通道号,如果是摄像头产品，则通道号为0，如果是nvr等产品，channel则为前端摄像头的索引
 * @param [in] code_stream   码流索引,如果支持主子码流，\c OVD_HIGH_STREAM 为主码流，
 *                           \c OVD_LOW_STREAM 为子码流，不支持多码流，默认为
 *                           \c OVD_HIGH_STREAM
 * @param [in] contentType   准备传送的内容，详见枚举值OVD_ContentType 音频、视频
 * @param [in] isIFrame      是否是I帧  0：不是 1：是
 * @param [in] contentData   发送数据的首字节指针
 * @param [in] dataLen  本次发送数据的长度
 * @param [in] timestamp     该帧时间戳(ms)，厂商可以传入绝对时间戳，也可以传入相对时间戳
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_AVPushData(IN OVD_int32 channel, IN OVD_int32 code_stream, IN OVDContentType contentType, IN OVD_bool isIFrame, IN OVD_uchar *contentData,
                                IN OVD_int32 dataLen, IN OVD_uint64 timestamp);

/**
 * @brief 音视频内容传送结束接口
 * @ingroup SDK流媒体
 * @details 该接口是同步接口，在弱网下因需要上传云存数据而阻塞一段时间。
 *
 * @param [in] channel   通道号,如果是摄像头产品，则通道号为0，如果是nvr等产品，channel则为前端摄像头的索引
 * @param [in] code_stream 码流索引,如果支持主子码流，\c OVD_HIGH_STREAM 为主码流，
 *                           \c OVD_LOW_STREAM 为子码流，不支持多码流，默认为
 *                           \c OVD_HIGH_STREAM
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_AVPushEnd(const IN OVD_int32 channel, const IN OVD_int32 code_stream);





/**
 * @brief SD卡录像内容回放推送接口（p2p 卡回放推流接口)
 * @ingroup 卡回放
 * @deprecated p2p业务弃用
 * @param [in] channel       通道号,如果是摄像头产品，则通道号为0，如果是nvr等产品，channel则为前端摄像头的索引
 * @param [in] contentType   准备传送的内容，详见枚举值OVD_ContentType 音频、视频
 * @param [in] isIFrame      是否是I帧  0：不是 1：是
 * @param [in] contentData   发送数据的首字节指针
 * @param [in] dataLen       本次发送数据的长度
 * @param [in] timestamp     该帧时间戳(ms)
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_SendRecordAVContent(IN OVD_int32 channel, IN OVDContentType contentType, IN OVD_bool isIFrame, IN OVD_void *contentData,
        IN OVD_int32 dataLen, IN OVD_uint64 timestamp);

/**
 * @brief SD卡录像内容回放推送完成 (p2p接口)
 * @ingroup 卡回放
 * @deprecated p2p业务弃用
 * @details 通知客户端录像文件播放完毕
 *
 * @param [in] channel   通道号,如果是摄像头产品，则通道号为0，如果是nvr等产品，channel则为前端摄像头的索引
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_RecordAVContentSendOver(IN OVD_int32 channel);

/// 兼容1.53之前版本符号
#define ovd_set_storage_info OVD_SetStorageInfo
#define ovd_set_storage_read_speed OVD_SetStorageReadSpeed
#define ovd_set_storage_encrypt_status OVD_SetStorageEncryptStatus
#define ovd_get_storage_encrypt_status OVD_GetStorageEncryptStatus
#define ovd_probe_exception_post OVD_ProbeExceptionPost
#define OVD_updatechannelstate OVD_UpdateChannelState

/**
 * @brief 设置卡状态
 * @ingroup 卡回放
 * @deprecated sdk内部卡存储机制暂未开放
 * @details 当开启sdk内部卡存储机制时，通过该函数通知sdk存储卡状态
 *
 * @param [in] channel   通道号
 * @param [in] info      卡状态信息
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_SetStorageInfo(const IN OVD_int32 channel, IN OvdLocalStorageInfo *info);

/**
 * @brief 设置卡回放速率
 * @ingroup 卡回放
 * @deprecated sdk内部卡存储机制暂未开放
 * @details 当开启sdk内部卡存储机制时，通过该函数通知sdk存储卡回放速率
 *
 * @param [in] channel   通道号
 * @param [in] speed     卡回放速率
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_SetStorageReadSpeed(const IN OVD_int32 channel, IN ovd_playback_speed_times_e speed);

/**
 * @brief 设置卡加密状态
 * @ingroup 卡回放
 * @deprecated sdk内部卡存储机制暂未开放
 * @details 当开启sdk内部卡存储机制时，通过该函数通知sdk存储卡加密状态
 *
 * @param [in] channel   通道号
 * @param [in] encrypt_status 卡加密状态
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_SetStorageEncryptStatus(const IN OVD_int32 channel, const IN OVD_int32 encrypt_status );

/**
 * @brief 获取卡加密状态
 * @ingroup 卡回放
 * @deprecated sdk内部卡存储机制暂未开放
 * @details 当开启sdk内部卡存储机制时，通过该函数获取sdk存储模块里的卡加密状态
 *
 * @param [in] channel   通道号
 * @param [out] encrypt_status 卡加密状态
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_GetStorageEncryptStatus(const IN OVD_int32 channel, OUT OVD_int32* encrypt_status );






/**
 * @brief 开始直播抓包模式 （涉及直播，对讲，云存）
 * @ingroup 调试
 * @details 调用此接口，OVD_AVPushData 接口会将音视频数据分别dump到两个文件输出文件为： \n
 *          - 音频：{path}/{filepre}_liveaudio.adts
 *          - 视频：{path}/{filepre}_live.video
 *
 *          该接口为不可重入函数接口，抓包模式下不建议切换h264,h265
 *
 * @param [in] filepath     文件输出路径
 * @param [in] filepre      文件输出前缀
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_StartLiveCapture(IN OVD_char *filepath, IN OVD_char *filepre);


/**
 * @brief 关闭直播抓包模式
 * @ingroup 调试
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_StopLiveCapture();


/**
 * @brief 开始直播抓包模式 （涉及直播，对讲，云存），支持按通道号抓取
 * @ingroup 调试
 * @details 调用此接口，OVD_AVPushData 接口会将音视频数据分别dump到两个文件输出文件为： \n
 *          - 音频：{path}/{filepre}_liveaudio.adts
 *          - 视频：{path}/{filepre}_live.video
 *
 *          该接口为不可重入函数接口，抓包模式下不建议切换h264,h265
 *
 * @param [in] channel 通道号
 * @param [in] stream  主码流或者子码流
 * @param [in] filepath 文件输出路径
 * @param [in] filepre 文件输出前缀
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_StartLiveCaptureEx(const IN OVD_int32 channel, const IN OVDCodeStream stream, IN OVD_char *filepath, IN OVD_char *filepre);

/**
 * @brief 关闭直播抓包模式，支持按通道号停止
 * @ingroup 调试
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_StopLiveCaptureEx(const IN OVD_int32 channel, const IN OVDCodeStream stream);



/**
 * @brief 开始卡回放抓包模式 （涉及卡回放）
 * @ingroup 调试
 * @details 输出文件为：\n
 *          - 音频：{path}/{filepre}_recordaudio.adts
 *          - 视频：{path}/{filepre}_record.video
 *
 *          该接口为不可重入函数接口，抓包模式下不建议切换h264,h265
 *
 * @param [in] filepath     文件输出路径
 * @param [in] filepre      文件输出前缀
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_StartRecordCapture(IN OVD_char *filepath, IN OVD_char *filepre);

/**
 * @brief 关闭卡回放抓包模式
 * @ingroup 调试
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */

OVD_EXPORT OVD_int32 OVD_StopRecordCapture();



/**
 * @brief 告警开始
 * @ingroup 告警
 * @details 设备检测到告警条件，触发告警。若此时与信令服务器未连接，则sdk会缓存这个信令
 *
 * @param [in] alarmInfo   报警信息结构体，注意startTimeStamp单位为s，绝对时间戳
 *
 * @return
 * -  0:  成功上报且信息缓存
 * - -1:  失败
 * -  1:  服务器连接异常但告警被缓存
 */
OVD_EXPORT OVD_int32 OVD_AlarmInfoStart(IN OVDUpLoadAlarmInfo *alarmInfo);

/**
 * @brief 告警结束
 * @ingroup 告警
 * @param [in] channel        通道号,如果是摄像头产品，则通道号为0，如果是nvr等产品，channel则为前端摄像头的索引
 * @param [in] alarmType      告警类型，详细可见枚举OVD_AlarmType
 * @param [in] endTimeStamp   报警结束时间戳(绝对时间戳)，单位为秒
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_AlarmInfoEnd(const IN OVD_int32 channel, const IN OVDAlarmType alarmType, const IN OVD_uint64 endTimeStamp);

#ifdef CONFIG_MODULE_AIDIY
/**
 * @brief AI DIY疑似告警数据上报接口
 * @ingroup AIDIY端云协同
 * @details 厂商上报疑似告警后，即可持续调用该接口，将所需的疑似告警数据以json格式上报AI平台，在AI平台二次确认满足阈值后，会告知SDK，后续SDK会停止上报
 *
 * @param [in]  OVD_AIDIY_Info 疑似告警信息
 * @return
 * - =  0:  成功
 * - = -1:  失败
 */
OVD_EXPORT OVD_int32 OVD_AIDIY_Report(IN OVD_AIDIY_Info *aidiy_info);
#endif

/**
 * @brief 获取SDK版本号信息
 * @ingroup 设备信息查询
 * @param [out] version 设备版本号
 * @param [in] version_len 调用者预分配的字符串长度
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_GetSDKVersion(OUT char *version, const IN int version_len);





/**
 * @brief 低功耗设备电量上报
 * @ingroup 低功耗
 * @details 设备上电会上报。OVD可以通过该方法向OVC上报当前的电量，一般电量百分比变化时
 *          可以发送该通知事件。设备可以决定每隔多少百分比上报一次电量，比如每隔10%上报一次。
 *
 * @param [in] battery    设备当前的电量百分比，整数： 0-100
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_BatteryChange(const IN OVD_int32 battery);



/**
 * @brief 上报异常信息
 * @ingroup 软探针
 * @param [in] event_info   异常上报，详情见结构体 \ref ovd_probe_except_info_t
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_ProbeExceptionPost(const IN ovd_probe_except_info_t event_info);





/**
 * @brief 动态加载视频通道
 * @ingroup 多通道
 * @param [in] channel 视频通道号
 * @param [in] cap     通道能力描述
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_InstallChannel(const IN int channel, IN OVD_ChannelCap_t *cap);

/**
 * @brief 动态卸载视频通道
 * @ingroup 多通道
 * @param [in] channel 视频通道号
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_UninstallChannel(const IN int channel);





/**
 * @brief 机器人告警信息上报
 * @ingroup 机器人业务
 * @details 如果告警上报失败，SDK不会缓存该告警，需要设备侧再次发送告警
 *
 * @param [in] alarmInfo   报警信息结构体
 *
 * @return
 * - =  0:  成功上报
 * - = -1:  失败
 */
OVD_EXPORT OVD_int32 OVD_HomibotEventNotify (IN OVDRobotAlarmInfo *alarmInfo);






/**
 * @brief 同步设备状态
 * @ingroup 设备状态变更
 * @details 设备状态变更。应用场景：\n
 *          - 设备升级
 *          - 设备异常
 *
 * @param [in] state
 *             - 1：设备在线
 *             - 2：设备升级
 *             - 3：异常
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_updateOVDstate(enum IN ovd_state state);

/**
 * @brief 同步设备channel状态
 * @ingroup 设备状态变更
 * @details channel状态变更。 应用场景：a.设备休眠；      b:设备唤醒
 *
 * @param [in] channel		   通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
 * @param [in] channelstate     0：离线;    1：在线;    3：异常;    4：不使能（休眠）;
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_UpdateChannelState(const IN OVD_int32 channel, enum IN ovd_channel_state channelstate);


/**
 * @brief 获取休眠服务器信息
 * @ingroup 低功耗
 * @param [out] hserverinfo 休眠服务器信息结构体
 *
 * @return
 * - 0：正常
 * - -1：异常
 */
OVD_EXPORT OVD_int32 OVD_GetHServerInfo(OUT ovd_hserverinfo_t *hserverinfo);





/**
 * @brief AI云存：通知sdk录像开启及关闭录制
 * @deprecated AIGC云存业务未开放
 * @details  通知sdk开始及结束录像
 *
 * @param [in] channel   通道号
 * @param [in] is_change ：检测到画面变化：ture:开始，false：停止。
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_aigc_detect_report(OVD_int32 channel, OVD_bool is_change);




typedef OVD_int32 (*OVD_callback_thirdparty_gettimeofday)(struct timeval *tv, struct timezone *tz);


/**
 * @brief 注册获取日期的函数
 * @ingroup 设备信息查询
 * @details 注册时机：若厂商 系统命令gettimeofday不支持或者有差异实现，可注册第3方实现
 *
 * @param [in] func 函数指针
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_register_thirdparty_gettimeofday(OVD_callback_thirdparty_gettimeofday func);



typedef struct
{
    /**
	* @brief [回调]打开AOV会话
	* @ingroup AOV
	* @details: 通知厂商打开一条某一channel下的AOV会话。
	* @param[in] channel    通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
	* @return
 	* -成功: 返回AOV会话厂商侧的上下文（context）标识，ctx，会话句柄非单个文件句柄
 	* -失败: NULL
	*/
    OVD_void * (*OVD_callback_AOV_open)(IN OVD_int32 channel);

    /**
     * @brief [回调]获取AOV码流信息
     * @ingroup AOV
     * @details 通知厂商获取AOV码流信息
     * @param[in] ctx  AOV会话厂商侧的上下文（context）标识
     * @param[in] info AOV码流信息，包含码流大小，帧数量，码流时长等
     * - = 0:  成功
 	 * - < 0:  失败*/
    OVD_int32 (*OVD_callback_AOV_get_info)(IN OVD_void *ctx, OVD_AOV_Info *info);

    /**
     *  @brief [回调]读取AOV数据
     *  @ingroup AOV
     *  @details 通知厂商读取AOV数据
     *  @param[in] ctx  AOV会话厂商侧的上下文（context）标识
     *  @param[in] pframe_info   帧信息，包含帧数据，帧类型，帧长度等
     *  @return
     * -OVD_RET_COMMON_ERROR = -1,    //通用错误码
     * -OVD_RET_SUCCESS = 0,          //成功
     * -OVD_RET_READ_FRAME_RETRY=201, //读帧重试错误码
     * -OVD_RET_READ_FRAME_EOF=202,   //AOV读取完毕，厂商反馈EOF
     *
    */
    OVD_int32 (*OVD_callback_AOV_readframe)(IN OVD_void *ctx, IN OVD_FrameInfo *pframe_info);

    /**
     * @brief [回调]设置AOV数据读取状态
     * @ingroup AOV
     * @details 在读取AOV数据的过程中，如果遇到异常通过该接口设置状态告知厂商，如果读完，也通过该接口告知厂商
     * @param[in] ctx  AOV会话厂商侧的上下文（context）标识
     * @param[in] status  读取的状态情况
     * @return
     * - = 0:  成功
 	 * - < 0:  失败
    */
    OVD_int32 (*OVD_callback_AOV_set_status)(IN OVD_void *ctx, OVD_aov_read_status_e status);

    /**
     * @brief [回调]关闭AOV会话
     * @ingroup AOV
     * @details 当AOV数据传完之后，会通知厂商关闭AOV会话，此时可以回收相关资源.【注意】不要在该回调中调用OVD_stop_push_AOV接口，请在该回调结束后再调用OVD_stop_push_AOV接口
     * @param[in] ctx  AOV会话厂商侧的上下文（context）标识
     * @return
     * - = 0:  成功
 	 * - < 0:  失败
    */
    OVD_int32 (*OVD_callback_AOV_close)(IN OVD_void *ctx);

}OVD_AOV_CallBackList;

/**
 *
 * @brief 当普通码流和AOV码流切换时，调用OVD_AVPushData接口推流，该接口用于判断此时是否可以往SDK推送AOV码流。【注意】SDK内部对于某些状态下是不允许推送AOV码流的，例如直播状态下等，因此在推送AOV码流之前都需要调用该接口进行确认。
 * @ingroup AOV
 * @param[in] channel 通道号
 * @param[in out] enable 是否可以推流标识，0表示不能推流，1表示可以推流
 * @return
 * - = 0:  成功
 * - < 0:  失败
*/
OVD_EXPORT OVD_int32 OVD_check_aov_push_enable(IN OVD_int32 channel, OVD_bool *enable);


/**
 *
 * @brief 启动低功耗设备AOV推流，需要SDK开始将AOV数据推流到云存储服务器。
 * @ingroup AOV
 * @note 需要在云存套餐下发之后再调用，否则会失败
 * @param[in] channel 通道号
 * @param[in] aov_cb AOV回调函数
 * @return
 * - = 0:  成功
 * - < 0:  失败
*/
OVD_EXPORT OVD_int32 OVD_start_push_AOV(IN OVD_int32 channel, IN OVD_AOV_CallBackList *aov_cb);

/**
 * @brief 停止低功耗设备AOV推流，在停止推流后或者需要休眠时，需要调用OVD_stop_push_AOV
 * @ingroup AOV
 * @param[in] channel 通道号
 * @return
 * - = 0:  成功
 * - < 0:  失败
*/
OVD_EXPORT OVD_int32 OVD_stop_push_AOV(IN OVD_int32 channel);


/**
 * @brief 设备云台状态信息上报
 * @ingroup 云台
 * @details 当触发OVD_PTZCmd回调时，若已达到极限转动角度/预置位设置错误/变焦达到极限/缩放首次达到极限时，调用此接口上报云台状态信息，或收到预置位设置/清除指令时，上报预置位信息
 * @note 该接口可重复调用，分别上报云台状态/预置位信息
 * @param[in] ptz_state
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_ptz_state_report(IN OVD_ptz_state_t *ptz_state);



/**
 * @brief 智能语音交互能力鉴权
 * @ingroup 智能语音交互
 * @details 设备通过该接口确认是否具备智能语音交互套餐
 * @param [in] license  设备侧license信息
 * @param [out] speechAuthResult 授权结果，0：鉴权成功，非0：鉴权失败
 *
 * @return
 * - =  0:  成功
 * - = -1:  失败,例如设备能力不支持智能语音对讲
 */
OVD_EXPORT OVD_int32 OVD_QuerySpeechAuthResult(IN OVD_char* license, OUT OVD_bool *speechAuthResult);

/**
 * @brief 智能语音端侧语音唤醒
 * @ingroup 智能语音交互
 * @details 设备通过该接口通知sdk即将开始语音交互会话 (异步接口)
 *
 * @param [out]sessionId 反馈会话id
 * @param [in]sessionId 分配的长度
 *
 * @return
 * - =  0:  成功
 * - = -1:  失败,例如设备能力不支持智能语音对讲
 */
OVD_EXPORT OVD_int32 OVD_SpeechAudioSessionOpen(OUT OVD_char* sessionId, IN OVD_int32 len);

/**
 * @brief 智能语音端侧关闭语音会话
 * @ingroup 智能语音交互
 * @details 通过调用该接口关闭当前语音会话，设备将停止推送音频数据
 * @param [in]sessionId 输入会话id
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_SpeechAudioSessionClose(IN  OVD_char* sessionId);

/**
 * @brief 智能语音端侧请求打断当前会话
 * @ingroup 智能语音交互
 * @details 智能语   音交互场景中，智能语音平台持续下发音频过程中，用户可能存在即时打断的需求。即设备可通过上报该指令，打断智能平台的持续音频下发
 * @param [in]sessionId 输入会话id
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_SpeechAudioSessionInterrupt(char* sessionId);
/**
 * @brief 音频准备（音频格式输入）
 * @ingroup 智能语音交互
 * @note 在通知语音回调媒体通道建立之后调用
 * @param [in] sessionId   会话id
 * @param [in] audeoinfo   音频信息,若无音频则为空（音频格式要求是AAC_with_ADTS）
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_SpeechMediaSendStart(IN const char* sessionId,IN OVDAudioDataFormat *audioinfo);


/**
 * @brief 智能语音端侧发送音频数据(音频格式固定为带adts头的aac 格式)
 * @ingroup 智能语音交互
 * @param [in] sessionId    会话id
 * @param [in] buf          音频数据
 * @param [in] len          音频数据长度
 * @param [in] timeStampMs	音频帧的时间戳

 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_SpeechMediaSend(IN const char* sessionId, IN  char* buf, IN unsigned int len,IN OVD_uint64 timeStampMs);

/**
 * @brief 智能语音端侧通知云端真实音频帧数据即将上传（同步接口)
 * @ingroup 智能语音交互
 * @param [in] sessionId    会话id

 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_SpeechAudioSessionNotify(char* sessionId);


#ifdef CONFIG_MODULE_logBuf
/**
 * @brief 日志实时上传到云平台的接口
 * @ingroup 日志
 * @param [in] level 日志等级
 * @param [in] tag 日志标签，可以指定为业务模块名称
 * @param [in] format 日志格式，参考printf
 *
 * @note 该打印接口的缓冲区长度为 2048 字节，通过format打印的日志长度不要超过该长度，否则会被内部截断。
 *
 * @return 0 成功
 *         -1 失败
 */
OVD_EXPORT OVD_int32 OVD_CaptureLogToCloud(OVDLogLevel level, const char *tag, const char *format, ...);
#endif




#ifdef CONFIG_WHITE_BOX
/**
 * @brief 上报设备许可证
 * @ingroup 白盒摄像头
 * @param[in] license 许可证
 * @return OVD_EXPORT
 */
OVD_EXPORT OVD_int32 OVD_NotifyAuthorizedStatus(const OVD_License *license);
#endif



#ifdef CONFIG_USING_CLOUD_AI
/**
 * @brief 上报算法包状态
 * @ingroup 动态算法
 * @param[in] uuid 平台操作标识符
 * @param[in] resource_info 内存信息
 * @param[in] process_pkg_info 进程包状态信息
 * @param[in] all_model_pkg_info 模型包状态信息
 * @return OVD_EXPORT
 */
OVD_EXPORT OVD_int32 OVD_ReportAllPackageStatus(char *uuid, OVD_ALG_DeviceResource resource_info, OVD_Alg_RunningInfo process_pkg_info, OVD_ALG_AllModelPackageInfo all_model_pkg_info);
#endif

/**
 * @brief 3d点云码流准备，该接口调用顺序在OVD_AVPushStart之后，确保OVD_AVPushStart调用成功后，如果设备支持3d点云，则调用该接口
 * @ingroup SDK流媒体
 * @param [in] channel     通道号,如果是摄像头产品，则通道号为0；如果是nvr等产品，channel则为前端摄像头的索引
 * @param [in] code_stream 码流（主码流为OVD_HIGH_STREAM，子码流为OVD_LOW_STREAM,不支持多码流默认为OVD_HIGH_STREAM）
 * @param [in] pointcloud_format   点云格式信息
 *
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
OVD_EXPORT OVD_int32 OVD_PointCloudPushStart(OVD_int32 channel, OVDCodeStream code_stream, const OVDPointCloudDataFormat* pointcloud_format);


#ifdef __cplusplus
}
#endif

#endif //OVDOPENAPI_OPENAPI_H
