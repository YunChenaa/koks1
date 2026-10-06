/*************************************************************
 *
 * This is a part of the CMCC RTC SDK.
 * Copyright (C) 2021 CMCC
 * All rights reserved.
 *
 *************************************************************/

#ifndef __CMCC_RTC_API_H__
#define __CMCC_RTC_API_H__

#ifdef __cplusplus 
extern "C" 
{ 
#endif

#define __cmcc_api__

/**
 * cmcc extended option
 */
typedef enum {
	/**
     * 设置自动接听的标识
	 * 说明：on_recv_call来电回调中，对于需要自动接听的来电，
	 * 则接听前需要先设置自动接听的标识。
	 * 具体参考提供的demo。
     */
	CMCC_OPT_AUTO_ANSWER_FLAG 			= 33, 
	/**
     * 使能H265编码协商
	 * 说明：SDK默认开启H265编码协商，如果不需要H265编码协商功能，
	 * 则需要关闭H265编码协商功能。
	 * 具体参考提供的demo。
     */
	CMCC_OPT_ENABLE_H265 				= 34,
	/**
	 * 设置主动挂断原因到SDK内部
	 * 说明：默认情况下，用户主动挂断时，不用设置此属性，以下情况需要设置
	 * 【1】被安防业务打断当前正在进行的通话时，调用挂断接口前，需要设置此属性，"1"表示被安防告警打断后的挂断；"2"表示被安防对讲打断后的挂断；
	 * 【2】设备设置了免打扰模式后，调用挂断接口挂断来电前，需要设置此属性，“3”表示请勿打扰挂断
	 * 具体参考提供的demo。
     */
	CMCC_OPT_HANGUP_WITH_REASON 		= 35,
	/**
	 * 使能音频OPUS编码协商功能
	 * 说明：默认情况下，SDK内部开启OPUS编码协商功能。
	 * 由于OPUS音频编码质量优于PCMA，故设备配置允许的情况下，建议不要关闭OPUS编码协商功能。
	 * 具体参考提供的demo。
     */
	CMCC_OPT_ENABLE_OPUS_NEGOTIATE 		= 36,
	/**
	* 设置设备支持的最高分辨率，标清(sd)，高清(hd)，超高清(fhd)
	* 底层默认使用标清，如有特殊分辨率要求（如门铃、门锁设备），可设置具体的宽高，如"width=640,height=480"
	* 具体使用参考提供的demo 
	*/
	CMCC_OPT_VIDEO_RESOLUTION 			= 37,
	/**
     * 设置政企模式
	 * 说明：默认情况下，不开启政企模式。
	 * 具体参考提供的demo。
     */
	CMCC_OPT_GOVERNMENT_ENTERPRISE 		= 38,
	/**
	 * 设置线程运行模式为轮询
	 * 1.5.3版本开始信令发送线程调度模型改为事件驱动（信号量），通过设置轮询模式可以回退到以前版本的轮询调度模型
	 * 事件驱动调度模型可以降低cpu使用率和功耗，除非出现调度BUG，否则不推荐回退为以前的轮询调度模型。
	 */
	CMCC_OPT_THREAD_MODE_LOOP 			= 39,

	/**
	 * 设置视频FEC功能
	 * "0":关闭 FEC; "1":打开FEC
	 */
	CMCC_OPT_VIDEO_FEC               	= 40,
	/**
	 * 设置视频分辨率竖屏模式
	 * "0":关闭竖屏模式; "1":打开竖屏模式， 底层默认都使用横屏模式，特殊厂家设备可开启竖屏模式
	 */
	CMCC_OPT_PORTRAIT					= 41,
	/**
	 * 开启或关闭接收DTMF按键声播放
	 * "0":关闭播放; "1":开启播放， 底层默认开启播放，特殊厂家设备可关闭DTMF按键播放
	 */
	CMCC_OPT_ENABLE_DTMF_TONE			= 42,
	/**
	 * 设置设备支持的远程控制能力对应的控制编码ID（参考demo使用）
	 * 控制台灯开关ID：100100；控制门锁开关门ID：100200；控制摄像头云台转动ID：100300
	 */
	CMCC_OPT_REMOTE_CONTROL_ID			= 43,
	/**
	 * 开启视频编码方式和分辨率合并回调的通知事件（见CMCC_NOTIFY_VIDEO_CODEC_AND_RESOLUTION回调通知）
	 * 说明：默认视频编码方式和分辨率分别单独回调给上层，开启此选项功能后，视频编码方式和分辨率合并回调给上层。
	 */
	CMCC_OPT_ENABLE_CODEC_AND_RESOLUTION_CALLBACK = 44,
	/**
	 * 设置双向视频通话模式
	 * "0":关闭双向视频通话; "1":打开双向视频通话， 底层默认关闭双向视频通话功能，特殊厂家设备可开启
	 */
	CMCC_OPT_RECV_VIDEO                  = 45,
	/**
	 * 关闭设备拉取通讯录功能
	 * "0":不关闭; "1":关闭
	 *
	 * 注意：sdk底层默认拉取通讯录，上传不需要通讯录可以选择关闭通讯录拉取功能，"1": 关闭拉取通讯录
	 */
	CMCC_OPT_DISENABLE_CONTACTS            = 46,
	/**
	 * 设置设备的flash路径, 以便存储DNS解析出来的IP地址，加快SDK启动速度
	 */
	CMCC_OPT_FLASH_DIR                     = 47,
	/**
	 * 获取设备固话号码
	 * 说明：
	 * [1]开通和家智话的手机用户才有固话号，未开通和家智话的用户没有固话号
	 * [2]开通和家智话“固话号码”业务时，返回指定的固话号码；
	 * [3]开通和家智话“随机号码”业务时，返回“random number”;
	 * [4]未开通和家智话业务时，返回空
	 */
	CMCC_OPT_FIXED_LINE_NUMBER             = 48,
	/**
	 * 设置厂商自己的完整日志文件路径（路径+日志文件名称），SDK上传和家智话日志时，同时将厂商日志文件上传至和家智话平台
	 * 厂商日志文件大小必须小于设置给和家智话的日志文件大小（cmcc_rtc_log_config_t中的log_size）
	 * 如厂商设置给和家智话的日志文件大小为1M，那么厂商自己的日志文件必须小于1M，否则无法上传厂商日志
	 */
	CMCC_OPT_LOG_PATH             		   = 49,
	/*
	 * 设置设备支持解码的分辨率，标清(sd)，高清(hd)，超高清(fhd)
	 * 底层默认使用标清，如有特殊分辨率要求（如门铃、门锁设备），可设置具体的宽高，如"width=640,height=480"
	 * 具体使用参考提供的demo 
	 */
	CMCC_OPT_RECV_RESOLUTION               = 50,
} cmcc_rtc_opt_;

/**
 * cmcc notify type
 */
typedef enum {
	CMCC_NOTIFY_UNBIND 				= 0,	//解绑
	CMCC_NOTIFY_BIND 				= 1,	//绑定
	CMCC_NOTIFY_ENABLE 				= 2,	//启动
	CMCC_NOTIFY_DISABLE 			= 3,	//禁用
	CMCC_NOTIFY_OPEN 				= 4,	//开户
	CMCC_NOTIFY_CLOSE 				= 5,	//销户
	CMCC_NOTIFY_CONTACT 			= 6,	//通信录变更
	
	CMCC_NOTIFY_LOGOUT              = 22,   //下线通告 DISCONNECT (此消息仅供复用安防TCP长连接方案使用，未复用安防TCP长连接方案情况下禁止处理此消息)
	CMCC_NOTIFY_REMOTE_CONTROL		= 33,	//远程控制通知
	CMCC_NOTIFY_REMOTE_QUERY		= 34,	//远程查询设备状态通知

	CMCC_NOTIFY_VIDEO_CODEC 		= 1001,	//视频编码方式
	CMCC_NOTIFY_VIDEO_RESOLUTION 	= 1002, //视频分辨率
	CMCC_NOTIFY_VIDEO_CODEC_AND_RESOLUTION = 1004, //视频编码方式和分辨率
	CMCC_NOTIFY_SWITCH_TO_AUDIO		= 1005,	//从视频通话切换到音频通话
} cmcc_notify_t;

/**
 * CMCC RTC SDK call type
 */
typedef enum {
    CMCC_CALL_TYPE_IMS_1V1_AUDIO = 20,		//音频呼叫类型
	CMCC_CALL_TYPE_IMS_1V1_VIDEO = 21,		//视频呼叫类型
} cmcc_call_type_t;

/**
 * CMCC RTC SDK event handler
 */
typedef struct {
	/**
     * 登录成功回调
	 * 
	 * @注意：不能在回调中做阻塞操作
     *
     * @param[out] user      登录成功的用户
     */
    void (*on_login_success)(const char *user);
	/**
     * 登录失败回调
	 * 
	 * @注意：不能在回调中做阻塞操作
     *
     * @param[out] user      登录失败的用户
	 * @param[out] err_code  登录失败的错误码
	 * @param[out] reason    登录失败的原因
     */
    void (*on_login_failed)(const char *user, int err_code, const char *reason);
	/**
     * 来电回调
     *
	 * @注意：不能在回调中做阻塞操作
	 * 
     * @param[out] session     			会话id
	 * @param[out] from  				来电号码
	 * @param[out] displayname  		来电号码
	 * @param[out] to           		被叫号码
	 * @param[out] call_type    		会话类型
	 * @param[out] json_call_control    会话控制
	 * @说明：
     * [1] json_call_control中有"answer_auto"字段，表示需要自动接听的来电，走自动接听逻辑；
     * [2] json_call_control中没有"answer_auto"字段，表示非自动接听的来电，走手动接听逻辑；
     * [3] json_call_control中有"app_broadcast"字段，表示云广播来电
     * [4] json_call_control中有"app_talk"字段，表示安防对讲来电
     * 
     * 备注：
     * [1] 对于需要自动接听的来电，先调用cmcc_rtc_setopt接口，将自动接听的flag设置到sdk层，
     *     然后再调用cmcc_rtc_pickup接口，接听来电；
     * [2] 对于未带answer_auto的来电，需要手动接听；
	 * 
     */
    void (*on_recv_call)(int session, const char *from, const char *displayname, const char *to, cmcc_call_type_t call_type, const char* json_call_control);
	/**
     * 回铃音回调（设备主动呼出时会收到此回调）
     *
	 * @注意：不能在回调中做阻塞操作
	 * 
     * @param[out] session     			会话id
	 * @param[out] from  				来电号码
	 * @param[out] displayname  		来电号码
	 * @param[out] to           		被叫号码
	 * @param[out] early_media    		早期媒体
	 * @说明：
	 * [1]early_media=1表示平台会播放回铃音，设备端不用自己播放回铃音；
	 * [2]early_media=0表示平台不播放回铃音，设备需要自己播放一个本地回铃音；
	 * [3]设备主动呼出后，此回调可能出现多次，如果early_media为0时，
	 *    设备自己播放本地回铃音；early_media为1时，设备停止播放本地回铃音。
	 * 
     */
    void (*on_recv_ring)(int session, const char *from, const char *displayname, const char *to, int early_media);
	/**
     * 会话接通回调（设备主动呼出后，被叫接听来电，设备会收到此回调）
     *
	 * @注意：不能在回调中做阻塞操作
	 * 
     * @param[out] session     			会话id
	 * @param[out] from  				来电号码
	 * @param[out] displayname  		来电号码
	 * @param[out] to           		被叫号码
	 * @param[out] call_type    		会话类型
	 * 
     */
    void (*on_recv_answer)(int session, const char* from, const char* displayname, const char* to, cmcc_call_type_t call_type);
	/**
     * 会话挂断回调
     *
	 * @注意：不能在回调中做阻塞操作
	 * 
     * @param[out] session    会话id
	 * @param[out] err_code   来电号码
	 * @param[out] reason     来电号码
	 * @param[out] call_type  会话类型
	 * 
     */
    void (*on_recv_hangup)(int session, int err_code, const char *reason, cmcc_call_type_t call_type);
	/**
     * 事件通知回调
	 * 
	 * @注意：不能在回调中做阻塞操作
	 * 
     * @param[out] notify_type    通知类型
	 * @param[out] content   	  通知内容
	 * 
     */
    void (*on_recv_notify)(cmcc_notify_t notify_type, const char *content);
	/**
     * 
	 * 关键帧请求回调
	 * @说明：如果接入厂家收到此回调事件后，需要发送一个关键帧给对端
	 * 
	 * @注意：不能在回调中做阻塞操作
	 * 
     */
	void (*on_recv_keyframe_request)();
	/**
     * dtmf通知回调
     * @说明：通话过程中，手机端点击的DTMF数字按键通过此接口回调给接入厂家
	 * 
	 * @注意：不能在回调中做阻塞操作
	 * 
     * @param[out] value    按键值
	 * 
     */
	void (*on_recv_dtmf)(int value);
	/**
     * 接收pcm音频数据回调
	 * @说明：
	 * 【编解码版本的SDK】使用此接口来接收PCM音频数据
	 * 发送PCM音频数据接口见 on_recv_audio_pcm_data
	 * 
	 * 特别注意：
     * 【编解码版本的SDK】音频流对接的方式有两种：（1）在third_party_device.cc文件中对接；
	 * （2）通过on_recv_audio_pcm_data和on_send_audio_pcm_data接口回调对接，具体使用见"SDK接口说明文档"
	 * 如果提供给厂家的demo压缩文件中有third_party_device.cc等文件，则使用第一种对接方式。
	 * 
     * @param[out] pcm_data  pcm音频数据（SDK回调给厂家的pcm数据）
	 * @param[out] size   	 数据长度（SDK回调给厂家的pcm数据长度）
	 * 
     */
	void (*on_recv_audio_pcm_data)(char *pcm_data, int size);
	/**
     * 发送pcm音频数据回调
	 * @说明：
	 * 【编解码版本的SDK】使用此接口发送原始的PCM音频数据
	 * 接收PCM音频数据接口见 on_recv_audio_pcm_data
     *
     * @param[out] pcm_data  pcm音频数据（厂家将需要发送的pcm数据拷贝到此参数，pcm_data必须为有效的正常数据）
	 * @param[out] size   	 数据长度（底层回调实际需要的pcm数据长度，厂家依据此参数来发送pcm数据的长度）
	 * 
     */
	void (*on_send_audio_pcm_data)(char *pcm_data, int size);
	/**
     * 编码后的音频数据回调
	 * @说明：
	 * 【非编解码版本的SDK】使用此接口接收编码后的音频数据
	 * 发送编码后的音频数据接口见 cmcc_rtc_send_audio
	 * 提供编码音频数据（如PCMA数据）的接入厂家，SDK收到的音频流数据，通过此回调接口回调给厂家；
	 * 默认提供的SDK都是编解码版本的SDK，提供原始音频数据（PCM数据）的接入厂家，忽略此回调接口和cmcc_rtc_send_audio接口
     *
     * @param[out] packet    音频数据包（编码的PCMA音频数据）
	 * @param[out] size   	 数据长度
	 * 
     */
	void (*on_recv_audio_packet)(char *packet, int size);
} cmcc_rtc_event_handler_t;

/**
 * CMCC RTC SDK login params
 */
typedef struct {
    char *app_key;				//Appkey
    char *app_secret;			//Appsecret
    char *device_id;			//组成格式为: cmcc-产品ID-设备序列号sn
} cmcc_rtc_login_params_t;

/**
 * CMCC RTC SDK device info
 */
typedef struct {
	char *os_version;			//操作系统版本号
    char *image_version;		//镜像版本号
    char *hardware_version;		//固件版本号
    char *device_model;			//设备信息
    char *sn;					//设备序列号
    char *cmei;					//设备cmei
    char *mac;					//设备mac地址
    char *chip_model;   		//芯片型号
    char *chip_uuid;    		//芯片序列号
	char *chip_brand;   		//芯片厂商
} cmcc_rtc_device_info_t;

/**
 * CMCC RTC SDK log config
 */
typedef struct {
	int log_size;					//日志大小
	int console_log_enable;			//是否开启控制台输出, 1.5.18版本开始release版本不在输出日志到控制台
	/**
	 * 
	 * @说明：
     * 【1】日志文件必须是txt格式
     * 【2】同时集成安防OVD和固话SDK的设备，需将固话日志、安防SDK日志以及设备相关日志放在同一个目录下，
     *     并且该目录下只能存放日志文件，安防日志上传时需将该目录下的所有日志文件打包成一个压缩文件上传。
     * 
     */
	char *log_path;			//日志存储路径

} cmcc_rtc_log_config_t;

/**
 * 初始化 CMCC RTC SDK.
 *
 * @param[in] device_info          设备信息
 * @param[in] log_config           日志配置信息
 *
 * @param[in] event_handler        SDK事件回调信息
 *
 * @return
 * - = 0: 成功
 * - < 0: 失败
 */
__cmcc_api__ int cmcc_rtc_init(const cmcc_rtc_device_info_t *device_info,
							   const cmcc_rtc_log_config_t *log_config,
							   const cmcc_rtc_event_handler_t *event_handler);
/**
 * 释放 CMCC RTC SDK 分配的资源
 *
 * @return
 * - = 0: 成功
 * - < 0: 失败
 */
__cmcc_api__ int cmcc_rtc_fini(void);
/**
 * 获取 CMCC RTC SDK 版本号
 *
 * @return CMCC RTC SDK 版本号
 */
__cmcc_api__ const char* cmcc_rtc_get_version(void);

#if 0
/**
 * 只获取账号信息,不登录 CMCC RTC 服务器，参数同cmcc_rtc_login（此接口仅供复用安防TCP长连接方案使用）
 *
 * @param[in] login_params      登录参数
 * 
 * @return
 * - = 0: 接口调用成功
 * - < 0: 接口调用失败
 * 		  说明：登录服务器成功与否，请参考相关登录事件回调
 */
__cmcc_api__ int cmcc_rtc_get_account(const cmcc_rtc_login_params_t *login_params);

/**
 * 登录 CMCC RTC 服务器（此接口仅供复用安防TCP长连接方案使用）
 *
 * @参数为空
 * 
 * @return
 * - = 0: 接口调用成功
 * - < 0: 接口调用失败
 * 		  说明：登录服务器成功与否，请参考相关登录事件回调
 */
__cmcc_api__ int cmcc_rtc_login2();
#endif

/**
 *  获取账号信息，同时登录 CMCC RTC 服务器
 *
 * @param[in] login_params      登录参数
 * 
 * @return
 * - = 0: 接口调用成功
 * - < 0: 接口调用失败
 * 		  说明：登录服务器成功与否，请参考相关登录事件回调
 */
__cmcc_api__ int cmcc_rtc_login(const cmcc_rtc_login_params_t *login_params);

#if 0
/**
 * 登出 CMCC RTC 服务器（此接口仅供复用安防TCP长连接方案使用）
 * 
 * @return
 * - = 0: 成功
 * - < 0: 失败
 */
__cmcc_api__ int cmcc_rtc_logout(void);
#endif

/**
 * 发起一个呼叫
 *
 * @param[in] callee      被叫号码 
 * 
 * @param[in] type        呼叫类型
 * 
 * @return
 * - >= 0: 接口调用成功，返回 session id
 * - < 0:  接口调用失败
 */
__cmcc_api__ int cmcc_rtc_callout(const char* callee, cmcc_call_type_t type);
/**
 * 接听来电通话
 *
 * @param[in] session      session id
 * 
 * @return
 * - = 0:  接口调用成功
 * - < 0:  接口调用失败
 */
__cmcc_api__ int cmcc_rtc_pickup(int session);
/**
 * 挂断通话
 *
 * @param[in] session      session id
 * 
 * @return
 * - = 0:  接口调用成功
 * - < 0:  接口调用失败
 */
__cmcc_api__ int cmcc_rtc_hangup(int session);
/**
 * 设置可选属性
 *
 * @param[in] opt      可选属性类型
 * 
 * @param[in] value    可选属性值
 * 
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
__cmcc_api__ int cmcc_rtc_setopt(cmcc_rtc_opt_ opt, const char* value);

/**
 * 获取属性对应的内容
 *
 * @param[in] opt      属性类型
 * @param[out] value   属性值
 * 
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
__cmcc_api__ int cmcc_rtc_getopt(cmcc_rtc_opt_ opt, char* value);

/**
 * 通过电话号码获取通信录对应的昵称
 *
 * @param[in] number   电话号码
 * 
 * @return             返回电话号码对应的昵称，无昵称则返回入参号码
 */
__cmcc_api__ const char* cmcc_rtc_get_nickname_by_number(const char* number);
/**
 * 通过通讯录昵称获取通讯录对应的电话号码
 * 
 * @param[in] nickname 通讯录昵称
 * 
 * @return  返回昵称对应的电话号码，若昵称不存在则返回错误告警    
 */
__cmcc_api__ const char* cmcc_rtc_get_number_by_nickname(const char* nickname);
/**
 * 上传SDK日志
 *
 * 
 * @return
 * - = 0:  成功
 * - < 0:  失败
 * 
 * @Deprecated: SDK内部已实现自动上传日志的功能
 * 
 */
__cmcc_api__ int cmcc_rtc_upload_logfile();
/**
 * 发送视频流接口
 *
 * @param[in] session   会话ID
 * @param[in] data      视频流（根据媒体协商结果发送H264或H265编码流）
 * @param[in] length    数据长度
 * 
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
__cmcc_api__ int cmcc_rtc_send_video(int session, char* data, int length);
/**
 * 发送编码后的音频数据接口
 * @说明：
 * 【非编解码版本的SDK】使用此接口发送编码后的音频流
 * 接受编码后的音频数据接口见 on_recv_audio_packet
 *
 * @param[in] session   会话ID
 * @param[in] data      编码后的音频数据（如PCMA编码音频数据）
 * @param[in] length    数据长度 （数据固定长度为160字节）
 * 
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
__cmcc_api__ int cmcc_rtc_send_audio(int session, unsigned char* data, int length);

/**
 * 发送DTMF按键消息
 * @说明：
 *
 * @param[in] session   会话ID
 * @param[in] code      对应键盘DTMF数字按键值[0-9、*(10)、#(11)]
 * 
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
__cmcc_api__ int cmcc_rtc_send_dtmf(int session, int code);

/**
 * 远程控制操作响应
 * @说明：需要设备端响应的远程控制操作，需调用此接口响应操作结果，如开锁是否成功（具体远程控制对接说明文档）
 *
 * @param[in] json_ack  json格式的远程控制响应指令
 * 
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
__cmcc_api__ int cmcc_rtc_remote_control_ack(char* json_ack);

/**
 * 上报设备状态信息，如开关状态，电量状态等数据
 * @说明：
 *
 * @param[in] json_device_states  json格式的设备状态信息
 * 
 * @return
 * - = 0:  成功
 * - < 0:  失败
 */
__cmcc_api__ int cmcc_rtc_report_device_states(char* json_device_states);

#ifdef __cplusplus
}
#endif	

#endif /* __CMCC_RTC_API_H__ */
