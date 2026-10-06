/*
 * Copyright (c) 2018. Sensetime Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sensetime Technology Co.
 */

/*
 * =====================================================================================
 *    Description:  Local message definitions
 *        Version:  1.0
 *         Author:  Xie, Hongbiao
 *   Organization:  Sensetime
 * =====================================================================================
 */

#ifndef __SENSE_LOCK_MESSAGE_L_H__
#define __SENSE_LOCK_MESSAGE_L_H__
#include "message_v2.h"

/*
 * Internal message and data structure definitions
 * which is not expected to be exposed to customer
 */

#define RGB_IMAGE_NAME_SIZE 32

/* communication message ID definitions */
const uint8_t MID_GETIMAGE = 0x14;               // to get the current image captured by the camera
const uint8_t MID_GETRAWIMAGE = 0x15;            // to get the raw image captured by the camera
const uint8_t MID_SNAP_DUAL_IMAGE = 0x29;        // to snap image from two cameras
const uint8_t MID_ENROLL_RGB = 0x1E;             // enroll with rgb image
const uint8_t MID_WRITE_SN = 0x31;               // write sn to board, for factory production
// const uint8_t MID_CHANGE_RELEASE_ENC_KEY = 0x54; // change release encrypted key
const uint8_t MID_FORMAT_FATFS = 0x56;           // format fatfs
const uint8_t MID_SET_RGBIMAGE_SIZE = 0x88;      // set rgbimage size
const uint8_t MID_TRANSFER_RGBIMAGE = 0x89;      // transfer rgbimage
const uint8_t MID_FACTORY_TEST = 0xC0;           // factory test
const uint8_t MID_DDR_TEST = 0xC1;               // DDR test
const uint8_t MID_SINGLE_TEST = 0xD0;            // Pick one image and save it
const uint8_t MID_CONTINUE_TEST = 0xD1;          // Pick images one by one
const uint8_t MID_STOP_TEST = 0xD2;              // Stop CONTINUE_TEST
// const uint8_t MID_REBOOT_CMD = 0xDA;             // reboot the device
/* communication message ID definitions END */

/* message result code */
const uint8_t MR_FAILED4_DSPRUNERROR = 11;   // feature matching failed, the name is just for consistency
const uint8_t MR_FAILED4_CAMERAFOV = 15;     // camera fov test failed
const uint8_t MR_FAILED4_CAMERABRT = 16;     // image brightness test failed
const uint8_t MR_FAILED4_CAMERASTRU = 17;    // camera structure test failed
const uint8_t MR_FAILED4_BOOT_TIMEOUT = 18;  // boot up timeout
const uint8_t MR_FAILED4_WITHOUT_SMILE = 22; // verify successful with smile
const uint8_t MR_FAILED4_NO_RGBIMAGE = 23;   // rgb image is not ready
const uint8_t MR_FAILED4_CAMERAVRF = 31;     // camera verify test failed
const uint8_t MR_FAILED4_FILE_CRC = 32;      // model file crc check failed
/* message result code END */

// Defined for MID_INIT_ENCRYPTION data:power_mode, for customizing the senseeye
// face recognition power mode in runing time. If not specified, senseeye will be
// runing in DEFAULT power mode. PM_LOW means lower power consumption but in the
// other hand it may cause a little bit more time. And PM_HIGH means the contrast.
const uint8_t PM_DEFAULT = 0; // balanced power and unlock performance
const uint8_t PM_LOW = 1;     // lower power but drop some unlock performance
const uint8_t PM_HIGH = 2;    // higher power with best unlock performance

/* msg face enroll type */
const uint8_t FACE_ENROLL_TYPE_RGB = 2; // enroll with one frame image
/* msg face enroll type END */

/* message: host -> module */
typedef struct {
    uint8_t ddr_test_counts; // number of DDR testing
} s_msg_ddr_test_data;

typedef struct {
    uint8_t enable_camera_test; // 0:disable camera test  !0:enable camera test
} s_msg_factory_test_data;

typedef struct {
    uint8_t media_type; // 0:audio 1:image 2:video 3:all
} s_msg_del_media_file_data;

typedef struct {
    uint8_t enable_pose_single; // enable pose detection in single test
} s_msg_single_test_data;

typedef struct {
    uint8_t enable_pose_continue; // enable pose detection in continue test
} s_msg_continue_test_data;

// set RGBImage Size
typedef struct {
    uint8_t rgbimage_size[4]; // image size int -> [s1, s2, s3, s4]
} s_msg_set_rgbimage_size_data;

// transfer rgbimage data
typedef struct {
    uint8_t rgbimage_offset[4]; // offset of rgbimage data for this package data
    uint8_t rgbimage_size[4];   // size of rgbimage data for this package data
    uint8_t data[0];
} s_msg_transfer_rgbimage_data;

// snap dual image data
typedef struct {
    uint8_t image_counts; // number of stored images
} s_msg_snap_dual_image_data;

/* message: module -> host */
typedef struct {
    uint8_t time_heb; // high eight bits of factory test time
    uint8_t time_leb; // low eight bits of facetory test time
} s_msg_reply_factory_test_data;

typedef struct {
    uint8_t image_size[4];  // image size int -> [s1, s2, s3, s4]
    uint8_t image2_size[4]; // image size int -> [s1, s2, s3, s4] for second image
} s_msg_reply_get_image_data;

typedef struct {
    uint8_t image_size[4]; // image size int -> [s1, s2, s3, s4]
} s_msg_reply_get_raw_image_data;

/* message: module -> host END */

#endif // __SENSE_LOCK_MESSAGE_L_H__
