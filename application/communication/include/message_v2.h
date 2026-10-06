/*
 * Copyright (c) 2021. Sensetime Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sensetime Technology Co.
 */

/*
 * =====================================================================================
 *    Description:  Communication message definitions
 *        Version:  2.0
 *         Author:  Xie, Hongbiao/Wang, Zaikuo
 *   Organization:  Sensetime
 * =====================================================================================
 */

#ifndef __SENSE_LOCK_MESSAGE_V2_H__
#define __SENSE_LOCK_MESSAGE_V2_H__
#include "koks_project_def.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define USER_NAME_SIZE 32
#define VERSION_INFO_BUFFER_SIZE 32
#define PLATFORM_NAME_SIZE 8
#define PROJECT_NAME_SIZE 8
#ifndef MAX_PALM_USER_COUNTS
#define MAX_PALM_USER_COUNTS 50
#endif
#ifndef MAX_FACE_USER_COUNTS
#define MAX_FACE_USER_COUNTS 50
#endif
#if MAX_PALM_USER_COUNTS > MAX_FACE_USER_COUNTS
#define MAX_USER_COUNTS MAX_PALM_USER_COUNTS
#else
#define MAX_USER_COUNTS MAX_FACE_USER_COUNTS
#endif
#define MAX_SN_SIZE 128
#define FEATURE_SIZE 1024
#define BASE64_FEATURE_SIZE 1368
#define AES_BASE64_FEATURE_SIZE 1376

/* communication message ID definitions*/
typedef uint8_t s_msg_id;
// Module to Host (m->h)
static const uint8_t MID_REPLY = 0x00; // request(command) reply message, success with data or fail with reason
static const uint8_t MID_NOTE = 0x01;  // note to host e.g. the position or angle of the face
static const uint8_t MID_IMAGE = 0X02; // send image to host
// Host to Module (h->m)
static const uint8_t MID_HW_CHECK = 0x0f;
static const uint8_t MID_RESET = 0x10;         // stop and clear all in-processing messages. enter standby mode
static const uint8_t MID_GETSTATUS = 0x11;     // to ping the module and get the status
static const uint8_t MID_VERIFY = 0x12;        // to verify the person in front of the camera
static const uint8_t MID_ENROLL = 0x13;        // to enroll and register the persion in front of the camera
static const uint8_t MID_ENROLL_SINGLE = 0x1D; // to enroll and register the persion in front of the camera, with one frame image
static const uint8_t MID_ENROLL_ITG = 0x26;    // Integrated enroll message, support all existing enroll type
static const uint8_t MID_SNAPIMAGE = 0x16;     // to snap a picture and save it
static const uint8_t MID_GETSAVEDIMAGE = 0x17; // to get size of saved image
static const uint8_t MID_UPLOADIMAGE = 0x18;   // upload images
static const uint8_t MID_SAVE_FEARURE = 0x19;
static const uint8_t MID_DELUSER = 0x20;                  // Delete the specified user with user id
static const uint8_t MID_DELALL = 0x21;                   // Delete all registerred users
static const uint8_t MID_GETUSERINFO = 0x22;              // Get user info
static const uint8_t MID_FACERESET = 0x23;                // Reset face status
static const uint8_t MID_GET_ALL_USERID = 0x24;           // get all users ID
static const uint8_t MID_GET_VERSION = 0x30;              // get version information
static const uint8_t MID_SET_VIDEO_PARAM = 0x36;          // set video param
static const uint8_t MID_SET_AUDIO_PARAM = 0x37;          // set audio param
static const uint8_t MID_SET_UVC_CONTROL = 0x38;          // set uvc param
static const uint8_t MID_SET_UVC_SPECIFIC_FRAME = 0x39;   // set uvc specific frame
static const uint8_t MID_UVC_SWITCH = 0x3B;               // set uvc switch
static const uint8_t MID_GET_VIDEO_MODE = 0x3D;           // get video mode
static const uint8_t MID_START_OTA = 0x40;                // ask the module to enter OTA mode
static const uint8_t MID_STOP_OTA = 0x41;                 // ask the module to exit OTA mode
static const uint8_t MID_GET_OTA_STATUS = 0x42;           // query the current ota status
static const uint8_t MID_OTA_HEADER = 0x43;               // the ota header data
static const uint8_t MID_OTA_PACKET = 0x44;               // the data packet, carries real firmware data
static const uint8_t MID_INIT_ENCRYPTION = 0x50;          // initialize encrypted communication
static const uint8_t MID_CONFIG_BAUDRATE = 0x51;          // config uart baudrate
static const uint8_t MID_SET_RELEASE_ENC_KEY = 0x52;      // set release encrypted key(Warning!!!:Once set, the KEY will not be able to modify)
static const uint8_t MID_SET_DEBUG_ENC_KEY = 0x53;        // set debug encrypted key
static const uint8_t MID_SET_UTC = 0x54;                  // set UTC time
static const uint8_t MID_REBOOT_CMD = 0x55;               // reboot the device
static const uint8_t MID_RESTORE_FACTORY_SETTINGS = 0x5A; // restore factory settings: delete all user's data
static const uint8_t MID_GET_LOGFILE = 0x60;              // get log file
static const uint8_t MID_UPLOAD_LOGFILE = 0x61;           // upload log file
static const uint8_t MID_EMOTION = 0x67;                  // play emotion or voice
#ifdef KOKS_SDK_VERSION_V2
static const uint8_t MID_PALM_ENROLL = 0x62;
#ifdef KOKS_FEATURE_KDS
static const uint8_t MID_PALM_VERIFY = 0x81;
static const uint8_t MID_PALM_DELALL = 0x82;
static const uint8_t MID_PALM_DELETE = 0x83;
static const uint8_t MID_PALM_QUERY = 0x84;
static const uint8_t MID_PALM_ENROLL_ITG = 0x85;
static const uint8_t MID_PALM_VERIFY_ONLY = 0x86;
#else
static const uint8_t MID_PALM_VERIFY_ONLY = 0x63;
static const uint8_t MID_PALM_QUERY = 0x64;
static const uint8_t MID_PALM_DELETE = 0x65;
static const uint8_t MID_PALM_DELALL = 0x66;
static const uint8_t MID_FACE_ENROLL_BYID = 0x85;         // enroll by id [admin, user name[32], direction, memberid[2], timeout, id]
#endif
static const uint8_t MID_PALM_SWITCH = 0x69;
static const uint8_t MID_MEMBERSHIP = 0x70;
#ifdef KOKS_FEATURE_DSM
static const uint8_t MID_VERIFY_FACE_SINGLE =0xA2;
#else
static const uint8_t MID_VERIFY_FACE_SINGLE = 0x72;
#endif
static const uint8_t MID_ENROLL_FACE_UP = 0x73;
static const uint8_t MID_ENROLL_FACE_DN = 0x74;
static const uint8_t MID_ENROLL_FACE_LEFT = 0x75;
static const uint8_t MID_ENROLL_FACE_RIGHT = 0x76;
static const uint8_t MID_ENROLL_FACE_SINGLE = 0x77;
#ifdef KOKS_FEATURE_KDS
static const uint8_t MID_PALM_ENROLL_SINGLE = 0x80;
#else
static const uint8_t MID_PALM_ENROLL_SINGLE = 0x78;
#endif
static const uint8_t MID_REPAIR_MODE = 0x71;              // repair mode
static const uint8_t MID_ENROLL_MULTI_MODE = 0x8E;        // enroll multimode [admin, user name[32], direction, memberid[2], timeout]
static const uint8_t MID_FACE_DEL_MULTI = 0x93;           // delete face by multi-id [id, id, ...]
static const uint8_t MID_FACE_ENROLL_BYID1 = 0x95;        // enroll face by id [admin, user name[32], direction, memberid[2], timeout, id]
static const uint8_t MID_FACE_ENROLL_BYID_NO_OVERWRITE = 0x97;// enroll face by id [admin, user name[32], direction, memberid[2], timeout, id]
#ifdef KOKS_FEATURE_KDS
static const uint8_t MID_PALM_ENROLL_BYID = 0x87;         // enroll palm by id [admin, user name[32], memberid[2], timeout, id]
#else
static const uint8_t MID_PALM_ENROLL_BYID = 0x86;         // enroll palm by id [admin, user name[32], memberid[2], timeout, id]
#endif
static const uint8_t MID_PALM_DEL_MULTI = 0x94;           // delete palm by multi-id [id, id, ...]
static const uint8_t MID_PALM_ENROLL_BYID1 = 0x96;        // enroll palm by id [admin, user name[32], memberid[2], timeout, id]
static const uint8_t MID_PALM_ENROLL_BYID_NO_OVERWRITE = 0x98; // enroll palm by id no overwrite [admin, user name[32], memberid[2], timeout, id]
#endif //KOKS_SDK_VERSION_V2

static const uint8_t MID_SET_THRESHOLD_LEVEL = 0xD4;      // Set threshold level
static const uint8_t MID_GET_THRESHOLD_LEVEL = 0xD5;      // Get threshold level
static const uint8_t MID_SET_THRESHOLD = 0xD6;            // Set threshold
static const uint8_t MID_GET_THRESHOLD = 0xD7;            // Get threshold
static const uint8_t MID_MISC_AI_CONFIG = 0xD8;           // MISC AI CONFIG
static const uint8_t MID_BLUR_CONFIG = 0xD9;              // BLUR CONFIG
#ifdef KOKS_FEATURE_PALM
static const uint8_t MID_PALM_LEVEL = 0xDA;               // palm level set/get
#endif //KOKS_FEATURE_PALM
static const uint8_t MID_POWERDOWN = 0xED;                // be prepared to power off
static const uint8_t MID_DEBUG_MODE = 0xF0;               // enter debug mode
static const uint8_t MID_GET_DEBUG_INFO = 0xF1;           // get size of debug information
static const uint8_t MID_UPLOAD_DEBUG_INFO = 0xF2;        // upload debug information
static const uint8_t MID_GETLIBRARY_VERSION = 0xF3;       // Get library version
static const uint8_t MID_GET_MODELS_MD5 = 0xF4;             // Get models md5
static const uint8_t MID_FACTORY = 0xFA;                  // factory mode message
static const uint8_t MID_DEMOMODE = 0xFE;                 // enter demo mode, verify flow will skip feature comparation step.
static const uint8_t MID_UVC_SWITCH2 = 0xFE;              // switch camera
static const uint8_t MID_MAX = 0xFF;                      // reserved
/* communication message ID definitions end */

/* message result code */
typedef uint8_t s_msg_result;
static const uint8_t MR_SUCCESS = 0;                // success
static const uint8_t MR_REJECTED = 1;               // module rejected this command
static const uint8_t MR_ABORTED = 2;                // algo aborted
static const uint8_t MR_FAILED4_CAMERA = 4;         // camera open failed
static const uint8_t MR_FAILED4_UNKNOWNREASON = 5;  // UNKNOWN_ERROR
static const uint8_t MR_FAILED4_INVALIDPARAM = 6;   // invalid param
static const uint8_t MR_FAILED4_NOMEMORY = 7;       // no enough memory
static const uint8_t MR_FAILED4_UNKNOWNUSER = 8;    // no user enrolled
static const uint8_t MR_FAILED4_MAXUSER = 9;        // exceed maximum user number
static const uint8_t MR_FAILED4_FACEENROLLED = 10;  // this face has been enrolled
static const uint8_t MR_FAILED4_LIVENESSCHECK = 12; // liveness check failed
static const uint8_t MR_FAILED4_TIMEOUT = 13;       // exceed the time limit
static const uint8_t MR_FAILED4_AUTHORIZATION = 14; // authorization failed
static const uint8_t MR_FAILED4_UNLOCK_FAIL = 16;   // verify/unlock failed
static const uint8_t MR_FAILED4_READ_FILE = 19;     // read file failed
static const uint8_t MR_FAILED4_WRITE_FILE = 20;    // write file failed
static const uint8_t MR_FAILED4_NO_ENCRYPT = 21;    // encrypt must be set
#ifdef KOKS_SDK_VERSION_V2
static const uint8_t MR_FAILED4_PALM_EXIST = 22;           // palm already exist
static const uint8_t MR_FAILED4_PALM_NONE = 23;            // no palm user
static const uint8_t MR_FAILED4_PALM_EXCEED_MAX = 24;      // palm user exceeded the maximum number of users
static const uint8_t MR_FAILED4_PALM_TIMEOUT = 25;         // palm lock/unlock timeout
static const uint8_t MR_FAILED4_PALM_ABORTED = 26;         // palm lock/unlock aborted
static const uint8_t MR_FAILED4_FACE_VER_ERR = 27;         // face modle version error
static const uint8_t MR_FAILED4_FACE_MODEL_TOO_LARGE = 28; // face modle too large
static const uint8_t MR_FAILED4_FACE_FUNC_ERR = 29;        // face modle interface error
static const uint8_t MR_FAILED4_PALM_MODEL_VER = 30;       // palm modle version error
static const uint8_t MR_FAILED4_PALM_MODEL_TOO_LARGE = 31; // palm modle too large
static const uint8_t MR_FAILED4_PALM_INTERFACE_ABNORMAL=32;// palm interface abnormal
static const uint8_t MR_FAILED4_PALM_INTERFACE_ERR = 33;   // palm interface error
static const uint8_t MR_FAILED4_FACE_HIGH_SIMILIAR = 34;   // face high similarity
static const uint8_t MR_FAILED4_PALM_MODLE_SWITCH = 35;    // palm modle switch
static const uint8_t MR_FAILED4_PALM_MISMATCH = 36;        // palm mismatch
static const uint8_t MR_FAILED4_INVALID_MEMBERID = 37;     // invalid member id
static const uint8_t MR_FAILED4_RELEASE_KEY_EXISTED = 38;  // release key existed
static const uint8_t MR_FAILED4_DUAL_FACE_VERIFY_FAIL = 39;// dual modle, face verify fail
static const uint8_t MR_FAILED4_DUAL_FACE_VERIFY_OK = 40;  // dual modle, face verify ok
static const uint8_t MR_FAILED4_DUAL_PALM_VERIFY_FAIL = 41;// dual modle, palm verify fail
static const uint8_t MR_FAILED4_DUAL_PALM_VERIFY_OK = 42;  // dual modle, palm verify ok
static const uint8_t MR_FAILED4_FACE_VERIFY_FAIL = 43;     // single modle, face verify fail
static const uint8_t MR_FAILED4_FACE_VERIFY_OK = 44;       // single modle, face verify ok
static const uint8_t MR_FAILED4_INTERRUPTED = 45;          // instruction execution process was interrupted‌
static const uint8_t MR_FAILED4_OTA_DATA_CHECK = 101;      // OTA checksum fail
static const uint8_t MR_FAILED4_OTA_PACKET_LOSS = 102;     // OTA packet loss
#else //KOKS_SDK_VERSION_V2
static const uint8_t MR_FAILED4_HW_FAILED = 22;     // hardware check failed
#endif //KOKS_SDK_VERSION_V2
/* message result code end */

/* module state */
typedef uint8_t s_mstate;
static const uint8_t MS_STANDBY = 0; // IDLE, waiting for commands
static const uint8_t MS_BUSY = 1;    // in working of processing commands
static const uint8_t MS_ERROR = 2;   // in error state. can't work properly
static const uint8_t MS_INVALID = 3; // not initialized
static const uint8_t MS_OTA = 4;     // in ota state
/* module state end */

// a general message adapter
typedef struct {
    uint8_t mid;      // the message id
    uint8_t size_heb; // high eight bits
    uint8_t size_leb; // low eight bits
    uint8_t data[0];
} s_msg;

/* message data definitions */
/* module -> host note */
/* msg note id */
typedef uint8_t s_note_id;
static const uint8_t NID_READY = 0;         // module ready for handling request (command)
static const uint8_t NID_FACE_STATE = 1;    // the detected face status description
static const uint8_t NID_UNKNOWNERROR = 2;  // unknown error
static const uint8_t NID_OTA_DONE = 3;      // OTA upgrading processing done
static const uint8_t NID_EYE_STATE = 4;     // the face in the eye_close status
#ifdef KOKS_FEATURE_DSM
static const uint8_t NID_PALM_STATUS = 8;   // the detected palm status description
#else
static const uint8_t NID_PALM_STATUS = 4;   // the detected palm status description
#endif
static const uint8_t NID_AUTHORIZATION = 8; // license authorizate failed
static const uint8_t NID_SNAP_STATE = 9;    // complete snap
static const uint8_t NID_FACE_PALAM = 0xf0;
/* msg note id end */

/* msg face direction */
typedef uint8_t s_face_dir;
static const uint8_t FACE_DIRECTION_UP = 0x10;       // face up
static const uint8_t FACE_DIRECTION_DOWN = 0x08;     // face down
static const uint8_t FACE_DIRECTION_LEFT = 0x04;     // face left
static const uint8_t FACE_DIRECTION_RIGHT = 0x02;    // face right
static const uint8_t FACE_DIRECTION_MIDDLE = 0x01;   // face middle
static const uint8_t FACE_DIRECTION_UNDEFINE = 0x00; // face undefine
/* msg face direction end */

/* msg face enroll type */
static const uint8_t FACE_ENROLL_TYPE_INTERACTIVE = 0; // interactive enroll
static const uint8_t FACE_ENROLL_TYPE_SINGLE = 1;      // enroll with one frame image
/* msg face enroll type end */

static const uint8_t FACE_STATE_NORMAL = 0;                     // normal state, the face is available
static const uint8_t FACE_STATE_NOFACE = 1;                     // no face detected
static const uint8_t FACE_STATE_TOOUP = 2;                      // face is too up side
static const uint8_t FACE_STATE_TOODOWN = 3;                    // face is too down side
static const uint8_t FACE_STATE_TOOLEFT = 4;                    // face is too left side
static const uint8_t FACE_STATE_TOORIGHT = 5;                   // face is too right side
static const uint8_t FACE_STATE_TOOFAR = 6;                     // face is too far
static const uint8_t FACE_STATE_TOOCLOSE = 7;                   // face is too near
static const uint8_t FACE_STATE_MULTI_FACE = 8;                 // 鹿客：有多个人
static const uint8_t FACE_STATE_EYEBROW_OCCLUSION = 8;          // eyebrow occlusion
static const uint8_t FACE_STATE_EYE_OCCLUSION = 9;              // eye occlusion
static const uint8_t FACE_STATE_FACE_OCCLUSION = 10;            // face occlusion
static const uint8_t FACE_STATE_DIRECTION_ERROR = 11;           // face direction error
static const uint8_t FACE_STATE_EYE_CLOSE_STATUS_OPEN_EYE = 12; // open eye in the eye close status
static const uint8_t FACE_STATE_EYE_CLOSE_STATUS = 13;          // confirm eye close status
static const uint8_t FACE_STATE_EYE_CLOSE_UNKNOW_STATUS = 14;   // eye close unknow status
static const uint8_t FACE_STATE_BLUR = 15;                      // blur
static const uint8_t FACE_STATE_COMPARE_FAIL = 16;             // 人脸库比对失败
static const uint8_t FACE_STATE_LOW_QUALATY = 17;              // 人脸质量过低
static const uint8_t FACE_STATE_2D_FAIL = 18;                  // 2D 活体失败
static const uint8_t FACE_STATE_3D_FAIL = 19;                  // 3D 活体失败
static const uint8_t FACE_STATE_ROTATE_LEFT = 20;              // 头部左转角度过大
static const uint8_t FACE_STATE_ROTATE_RIGHT = 21;             // 头部右转角度过大
static const uint8_t FACE_STATE_PITCH_UP = 22;                 // 头部上仰角度过大
static const uint8_t FACE_STATE_PITCH_DOWN = 23;               // 头部下仰角度过大
static const uint8_t FACE_STATE_HIGH_SIMILARATY_FAIL = 24;     // 高相似比对失败
static const uint8_t FACE_STATE_FRONT = 25;                    // 正脸

static const uint8_t PALM_STATE_NORMAL = 40;                   // 手掌正常
static const uint8_t PALM_STATE_NO_PALM = 41;                  // 未检测到手掌
static const uint8_t PALM_STATE_PALM_RECT_CONF_LOW = 42;       // 手掌框关键点置信度过低
static const uint8_t PALM_STATE_PALM_LANDMARK_CONF_LOW = 43;   // 手掌关键点置信度过低
static const uint8_t PALM_STATE_PALM_ANGLE_ROLL_ERROR = 44;    // 角度不符合要求——侧倾
static const uint8_t PALM_STATE_PALM_ANGLE_LEAN_ERROR = 45;    // 角度不符合要求——前倾
static const uint8_t PALM_STATE_PALM_BLOCK_1 = 46;             // 手掌遮挡 1
static const uint8_t PALM_STATE_PALM_BLUR_1 = 47;              // 手掌模糊 1
static const uint8_t PALM_STATE_PALM_BACK = 48;                // 手背
static const uint8_t PALM_STATE_PALM_GESTURE = 49;             // 异常手势
static const uint8_t PALM_STATE_PALM_OVER_EXPOSED = 50;        // 过曝光，未通过质量分
static const uint8_t PALM_STATE_PALM_UNDER_EXPOSED = 51;       // 欠曝光，未通过质量分
static const uint8_t PALM_STATE_PALM_BLOCK_2 = 52;             // 手掌遮挡 2
static const uint8_t PALM_STATE_PALM_BLUR_2 = 53;              // 手掌模糊 2
static const uint8_t PALM_STATE_PALM_WATER = 54;               // 手掌有水
static const uint8_t PALM_STATE_PALM_TOO_NEAR = 56;            // 手掌太近
static const uint8_t PALM_STATE_PALM_TOO_FAR = 57;             // 手掌太远
static const uint8_t PALM_STATE_PALM_IR_NOT_LIVE = 58;         // 活体不过
static const uint8_t PALM_STATE_COMPARE_FAIL = 59;             // 手掌比对失败
static const uint8_t PALM_STATE_REFLECT = 60;                  // 手掌反光
static const uint8_t PALM_STATE_OUT_LEFT = 61;                 // 手掌左侧出框
static const uint8_t PALM_STATE_OUT_RIGHT = 62;                // 手掌右侧出框
static const uint8_t PALM_STATE_OUT_TOP = 63;                  // 手掌顶部出框
static const uint8_t PALM_STATE_OUT_BOTTOM = 64;               // 手掌底部出框
static const uint8_t PALM_STATE_OUT_LT = 65;                   // 手掌左和上出框
static const uint8_t PALM_STATE_OUT_LB = 66;                   // 手掌左和下出框
static const uint8_t PALM_STATE_OUT_RT = 67;                   // 手掌右和上出框
static const uint8_t PALM_STATE_OUT_RB = 68;                   // 手掌右和下出框
static const uint8_t PALM_STATE_PALM_MOVE = 80;                // 手掌动一动


/* enc key type */
static const uint8_t RELEASE_ENC_KEY = 0; // for release
static const uint8_t DEBUG_ENC_KEY = 1;   // for debug
/* enc key type end */

typedef struct {
    int16_t state; // corresponding to FACE_STATE_*

    // position
    int16_t left; // in pixel
    int16_t top;
    int16_t right;
    int16_t bottom;

    // pose
    int16_t yaw;   // up and down in vertical orientation
    int16_t pitch; // right or left turned in horizontal orientation
    int16_t roll;  // slope
} s_note_data_face;

typedef struct {
    int16_t state; // corresponding to PALM_STATE_*
    int16_t far;
    int16_t close;
} s_note_data_palm;

typedef struct {
    uint8_t nid; // note id
    uint8_t data[0];
} s_msg_note_data;
/* module -> host note end */

// enroll user
typedef struct {
    uint8_t admin; // the user will be set to admin
    uint8_t user_name[USER_NAME_SIZE];
    s_face_dir face_direction; // depleted, user ignore this field
    uint8_t timeout;           // timeout, unit second default 10s
} s_msg_enroll_data;

// enroll user by feature
typedef struct {
    uint8_t admin; // the user will be set to admin
    uint8_t user_name[USER_NAME_SIZE];
    s_face_dir face_direction; // depleted, user ignore this field
    uint8_t timeout;           // timeout, unit second default 10s
    uint8_t aes_feature[AES_BASE64_FEATURE_SIZE];
} s_msg_enroll_feature_data;

// enroll user intergrated
typedef struct {
    uint8_t admin; // the user will be set to admin, 1:admin user, 0:normal user
    uint8_t user_name[USER_NAME_SIZE];
    uint8_t face_direction; // depleted, user ignore this field
    uint8_t enroll_type;
    uint8_t enable_duplicate; // enable user enroll duplicatly, 1:enable, 0:disable
    uint8_t timeout;          // timeout unit second default 10s
    uint8_t reserved[3];      // reserved feild
} s_msg_enroll_itg;

// verify
typedef struct {
    uint8_t pd_rightaway; // power down right away after verifying
    uint8_t timeout;      // timeout, unit second, default 10s
} s_msg_verify_data;

// delete user
typedef struct {
    uint8_t user_id_heb; // high eight bits of user_id to be deleted
    uint8_t user_id_leb; // low eight bits pf user_id to be deleted
} s_msg_deluser_data;

// get user info
typedef struct {
    uint8_t user_id_heb; // high eight bits of user_id to get info
    uint8_t user_id_leb; // low eight bits of user_id to get info
} s_msg_getuserinfo_data;

// start ota data MID_START_OTA
typedef struct {
    uint8_t v_primary;   // primary version number
    uint8_t v_secondary; // secondary version number
    uint8_t v_revision;  // revision number
} s_msg_startota_data;

// ota header MID_OTA_HEADER
typedef struct {
    uint8_t fsize_b[4];  // OTA FW file size int -> [b1, b2, b3, b4]
    uint8_t num_pkt[4];  // number packet to be divided for transferring, int -> [b1, b2, b3, b4]
    uint8_t pkt_size[2]; // raw data size of single packet
    uint8_t md5_sum[32]; // md5 check sum
} s_msg_otaheader_data;

// ota packet MID_OTA_PACKET
typedef struct {
    uint8_t pid[2];   // the packet id
    uint8_t psize[2]; // the size of this package
    uint8_t data[0];  // data 0 start
} s_msg_otapacket_data;

// MID_INIT_ENCRYPTION data version2
typedef struct {
    uint8_t seed[4];     // random data as a seed
    uint8_t mode;        // reserved for selecting encrytion mode
    uint8_t crttime[4];  // current time - this is the interface that module sync time with host
    uint8_t power_mode;  // depleted, not used at V2
    uint8_t reserved[2]; // reserved fields for some other control extension
} s_msg_init_encryption_data_v2;

// debug mode data
typedef struct {
    uint8_t enable; // enable debug mode or not
} s_msg_debug_mode_data;

// snap image data
typedef struct {
    uint8_t sensor_id;
    uint8_t image_counts; // number of stored images
    uint8_t start_number; // start number of stored images
} s_msg_snap_image_data;

// get saved image data
typedef struct {
    uint8_t image_number; // number of saved image
} s_msg_get_saved_image_data;

// upload image
typedef struct {
    uint8_t upload_image_offset[4]; // upload image offset, int -> [o1 o2 o3 o4]
    uint8_t upload_image_size[4];   // upload image size, int -> [s1 s2 s3 s4]
} s_msg_upload_image_data;

// upload debug informaiton
typedef struct {
    uint8_t upload_debug_info_offset[4]; // upload debug informaiton offset, int -> [o1 o2 o3 o4]
    uint8_t upload_debug_info_size[4];   // upload debug informaiton size, int -> [s1 s2 s3 s4]
} s_msg_upload_debug_info_data;

// upload logfile
typedef struct {
    uint8_t upload_logfile_offset[4]; // upload logfile offset, int -> [o1 o2 o3 o4]
    uint8_t upload_logfile_size[4];   // uploade logfile size, int -> [s1 s2 s3 s4]
} s_msg_upload_logfile_data;

// play emotion or voice
typedef struct {
    uint8_t emotion_id;         // emotion id, 0 means none.
    uint8_t voice_id;           // voice id, 0 means none.
    uint8_t default_emotion_id; // set the default emotion id to be played at boot time. 0 means none.
} s_msg_eomtion_data;

/* message reply data definitions */
typedef struct {
    uint8_t user_id_heb;
    uint8_t user_id_leb;
    uint8_t user_name[USER_NAME_SIZE];
    uint8_t admin;
    uint8_t unlockStatus;
} s_msg_reply_verify_data;

typedef struct {
    uint8_t user_id_heb;
    uint8_t user_id_leb;
    uint8_t face_direction; // depleted, user ignore this field
} s_msg_reply_enroll_data;

typedef struct {
    uint8_t status;
} s_msg_reply_getstatus_data;

typedef struct {
    uint8_t version_info[VERSION_INFO_BUFFER_SIZE];
} s_msg_reply_version_data;

typedef struct {
    uint8_t library_version_info[20];
} s_msg_reply_library_version_data;

typedef struct {
    uint8_t user_id_heb;
    uint8_t user_id_leb;
    uint8_t user_name[USER_NAME_SIZE];
    uint8_t admin;
} s_msg_reply_getuserinfo_data;

typedef struct {
    uint8_t user_counts;                   // number of enrolled users
    uint8_t users_id[MAX_USER_COUNTS * 2]; // use 2 bytes to save a user ID and save high eight bits firstly
} s_msg_reply_all_userid_data;

// MID_INIT_ENCRYPTION reply
typedef struct {
    uint8_t device_id[20]; // the unique ID of this device, string type
} s_msg_reply_init_encryption_data;

// REPLY MID_GET_OTA_STATUS
typedef struct {
    uint8_t ota_status;    // current ota status
    uint8_t next_pid_e[2]; // expected next pid, [b0,b1]
} s_msg_reply_getotastatus_data;

typedef struct {
    uint8_t image_size[4]; // image size int -> [s1, s2, s3, s4]
} s_msg_reply_get_saved_image_data;

typedef struct {
    uint8_t debug_file_size[4]; // debug file size int -> [s1, s2, s3, s4]
} s_msg_reply_get_debug_info_data;

typedef struct {
    uint8_t mid;    // the command(message) id to reply (the request message ID)
    uint8_t result; // command handling result: success or failed -> s_msg_result
    uint8_t data[0];
} s_msg_reply_data;

typedef struct {
    uint8_t ota_result; // 0 :is sucess; 1: is faile;
} s_note_ota_result;

typedef struct {
    uint8_t baudrate_index; // 1: is 115200 (115200*1); 2 is 230400 (115200*2); 3 is 460800 (115200*4); 4 is 1500000
} s_note_config_baudrate;

typedef struct {
    uint8_t enc_key_number[16]; //
} s_msg_enc_key_number_data;

typedef struct {
    uint8_t log_size[4]; // log size int -> [s1, s2, s3, s4]
} s_msg_reply_get_log_data;

typedef struct {
    uint8_t version_info[VERSION_INFO_BUFFER_SIZE];
    uint8_t support_factory_test; // 0:not support; 1:support
    uint8_t platform_name[PLATFORM_NAME_SIZE];
    uint8_t project_name[PROJECT_NAME_SIZE];
}s_msg_reply_factroy_data;

typedef struct {
    uint8_t verify_threshold_level;   // level 0~4, safety from low to high, default 2
    uint8_t liveness_threshold_level; // level 0~4, safety from low to high, default 2
} s_msg_algo_threshold_level;

typedef struct {
    float verify_threshold;
    float liveness_threshold;
} s_msg_algo_threshold;

typedef struct {
    uint8_t beard_enable; // 0不使能大胡子 1使能大胡子
    uint8_t device_type;  // 0门锁模式 1门铃模式
} s_msg_misc_ai_config;

#pragma pack(push)
#pragma pack(1)
typedef struct {
    uint8_t enable;
    float blur_threshold;
} s_msg_blur_config;
#pragma pack(pop)

typedef struct {
    s_msg_misc_ai_config misc;
    s_msg_blur_config blur;
} misc_config;

typedef struct {
    uint8_t admin; // the user will be set to admin
    uint8_t user_name[USER_NAME_SIZE];
    uint8_t feature[FEATURE_SIZE];
} s_msg_feature_data;

typedef struct {
    uint8_t camera_type;        //摄像头类型：0：主摄像头，1：副摄像头
    uint8_t stream_type;        //码流类型：0：主码流，1：子码流
    uint8_t encode;             //编码类型：0：H264，1：H265，2：MJPEG
    uint16_t resolution_width;  //分辨率的宽：例如 1920
    uint16_t resolution_height; //分辨率的高：例如 1080
    uint8_t framerate;          //帧率：10~30fps
    uint16_t bitrate;           //码率：100~65535（kbps）
    uint8_t save_flag;          //是否断电保存：0：不保存，1：保存
} cam_msg_video_param_data;

/*设置音频参数(0x37)*/
typedef struct {
    uint8_t mic_samplerate;       //麦克风的输入采样率：0:8000，1:16000
    uint8_t mic_payload_type;     //麦克风输入的载荷类型：0：PCM，1：AAC
    uint8_t mic_volume;           //麦克风的音量设置：0~100
    uint8_t speaker_samplerate;   //喇叭的输出采样率：0：8000，1：16000
    uint8_t speaker_payload_type; //喇叭的输出的载荷类型：0：PCM，1：AAC
    uint8_t speaker_volume;       //喇叭的音量设置：0~100
    uint8_t audio_denoisel_level; //回声消除的级别：0~10
    uint8_t save_flag;            //是否断电保存：0：不保存，1：保存
} cam_msg_audio_param_data;

/*请求视频帧数据（0x39）*/
typedef struct {
    uint8_t stream_type;      //码流类型：0：主码流，1：子码流
    uint8_t encode;           //编码类型：0：H264，1：H265，2：MJPEG
    uint8_t video_frame_type; //视频帧的类型：0：I 帧，1：P 帧
} cam_msg_req_video_data;

/*切换视频画面（0x3B）*/
typedef struct {
    uint8_t camera_type; //需要切换到的摄像头画面：0：主摄像头，1：副摄像头，2：主副摄像头合并（副摄像头在小框），3：主副摄像头合并（主摄像 头在小框）
    uint8_t uvc_flow;    //需要切换的 uvc：0：H264，1：H265，2：MJPEG
} cam_msg_uvc_switch_param_data;

/*切换视频画面（0xFE）*/
typedef struct {
    uint8_t camera_type; //需要切换到的摄像头画面：0: merge，1：up，2：down
} cam_msg_uvc_switch2_param_data;

typedef struct {
    uint8_t timestamp[4];
} cam_msg_sync_time_data;

typedef enum {
    TYPE_UART = 0,
    TYPE_CDC,
} s_type;
#endif // __SENSE_LOCK_MESSAGE_V2_H__
