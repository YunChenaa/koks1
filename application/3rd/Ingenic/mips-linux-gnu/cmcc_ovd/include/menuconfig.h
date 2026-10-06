/*
 * Automatically generated C config: don't edit
 */
/*
 * Version Number
 */
#ifndef OVDCFG_MODULE_SWITCH
#define OVDCFG_MODULE_SWITCH

/*
 * SDK base info configuration
 */
#define CONFIG_SDK_VERSION "1.58"

/*
 * SDK basic series
 */
#define CONFIG_SDK_SERIES_GENERAL 1
#define CONFIG_SDK_SERIES "general"

/*
 * Basic components configuration
 */

/*
 * Crypto options
 */
#define CONFIG_MODULE_mbedtls 1
#define CONFIG_MBEDTLS_VERSION_2_28_1 1
#define CONFIG_MBEDTLS_VERSION "2.28.1"

/*
 * Configuration validation
 */
#define CONFIG_MODULE_timer 1
#define CONFIG_MODULE_ts 1

/*
 * HTTP protocol stack
 */
#define CONFIG_MODULE_HTTPCLIENT 1
#define CONFIG_MODULE_libwebsocket 1
#define CONFIG_MODULE_cJSON 1

/*
 * Core components configuration
 */

/*
 * Media components
 */
#define CONFIG_MAX_CHANNEL_COUNT 1

/*
 * live streaming
 */
#define CONFIG_MODULE_srt 1
#define CONFIG_MODULE_srtclient 1
#define CONFIG_MODULE_cseg 1

/*
 * card storage
 */
#define CONFIG_MOUDLE_THIRDPARTY_STORAGE 1

/*
 * Card playback
 */
#define CONFIG_MODULE_CardPlayback 1
#define CONFIG_SDK_REPLAY_PROTOCOL_SRT 1
#define CONFIG_SDK_REPLAY_PROTOCOL "SRT"
#define CONFIG_MODULE_MediaBuffer 1

/*
 * Talk module
 */
#define CONFIG_SDK_TALK_PROTOCOL_CCRTC 1
#define CONFIG_SDK_TALK_PROTOCOL "CCRTC"
#define CONFIG_MODULE_picupload 1

/*
 * Network configuration options
 */
#define CONFIG_MODULE_ble 1
#define CONFIG_MODULE_QRstringanalyse 1
#define CONFIG_MODULE_serviceschedule 1
#define CONFIG_MODULE_DME 1
#define CONFIG_MODULE_OPENAPI 1

/*
 * Device manager options
 */

/*
 * Network Detection
 */
#define CONFIG_MODULE_softprobe 1

/*
 * log module
 */
#define CONFIG_MODULE_log 1
#define CONFIG_MODULE_inotify 1
#define CONFIG_MODULE_local_log 1
#define CONFIG_LOGFILE_SIZE_1MB 1
#define CONFIG_LOGFILE_SIZE 262144
#define CONFIG_MODULE_softwdt 1

/*
 * system configuration
 */
#define CONFIG_MEMORY_OPTIMIZATION 1
#define CONFIG_SDK_IPV6_MODE 1
#define CONFIG_ENCRYPTION_MODE 1

/*
 * Developer options
 */
#define CONFIG_ENABLE_TIMESTAMP_CHECK 1

/*
 * Host system configuration
 */
#define CONFIG_M64_MODE 1

/*
 * select Host system 
 */
#define CONFIG_TARGET_SYSTEM_LINUX 1
#define CONFIG_TARGET_SYSTEM "linux"

/*
 * select Host platform 
 */
#define CONFIG_CUSTOM_NAME "general"
#define CONFIG_PLATFORM_NAME "x86"

/*
 * Output configuration
 */
#define CONFIG_BUILD_SHARDLIB 1

#endif
