#pragma once

#define DEV_FRAME_SOURCE_0 0 // IR frame source group 1 ir nv12
#define DEV_FRAME_SOURCE_1 1 // RGB frame source group 5 rgb nv12
#define DEV_FRAME_SOURCE_2 2 // RGB frame source group 4 rgb h264
#define DEV_FRAME_SOURCE_3 3 // RGB frame source group 3 rgb mjpeg
#define DEV_FRAME_SOURCE_4 4 // IR frame source group 2 rgb h264
#define DEV_FRAME_SOURCE_5 5 // IR frame source group 0 rgb mjpeg
#define DEV_FRAME_SOURCE_6 6 // IR frame source group 2 rgb h264
#define DEV_FRAME_SOURCE_7 7 // IR frame source group 0 rgb mjpeg

#define IR_MJPEG_FRAME_SOURCE   DEV_FRAME_SOURCE_5
#define RGB_MJPEG_FRAME_SOURCE DEV_FRAME_SOURCE_3
#define IR_H264_FRAME_SOURCE   DEV_FRAME_SOURCE_4
#define RGB_H264_FRAME_SOURCE DEV_FRAME_SOURCE_2

#define DOWN_MJPEG_FRAME_SOURCE DEV_FRAME_SOURCE_6
#define DOWN_H264_FRAME_SOURCE DEV_FRAME_SOURCE_7

#define DUMMY_FRAME_SOURCE  0xFF

#define DEV_VENC_0 0 // RGB encode CHN 4 mjpeg
#define DEV_VENC_1 1 // RGB encode CHN 3 h264
#define DEV_VENC_2 2 // IR encode CHN 2 mjpeg
#define DEV_VENC_3 3 // IR encode CHN 0 h264
#define DEV_VENC_4 4 // DOWN encode CHN 0 mjpeg
#define DEV_VENC_5 5 // DOWN encode CHN 0 h264

#define DEV_OSD_0 0 // OSD device
