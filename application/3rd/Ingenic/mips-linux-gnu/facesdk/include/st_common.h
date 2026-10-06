#ifndef ST_COMMON_DEFINE_H_
#define ST_COMMON_DEFINE_H_
#include "st_utils.h"
/// handle declearation
typedef void *st_handle_t;

/// result declearation
typedef int st_result_t;

#define ST_FACE_LANDMARK_POINTS_CNT 106

/// rectangle definition
typedef struct {
  int left;    ///< 矩形最左边的坐标
  int top;     ///< 矩形最上边的坐标
  int right;   ///< 矩形最右边的坐标
  int bottom;  ///< 矩形最下边的坐标
} st_rect_t;

/// st float type point definition
typedef struct {
  float x;  ///< 点的水平方向坐标, 为浮点数
  float y;  ///< 点的竖直方向坐标, 为浮点数
} st_pointf_t;

/// @brief  人脸朝向,以头顶为标准
typedef enum {
  ST_FACE_UP = 0x0000001,  ///< 人脸向上，头顶向上
  ST_FACE_LEFT = 0x0000002,  ///< 人脸向左，即人脸被逆时针旋转了90度，头顶向左
  ST_FACE_DOWN = 0x0000004,  ///< 人脸向下，即人脸被逆时针旋转了180度，头顶向下
  ST_FACE_RIGHT = 0x0000008,  ///< 人脸向右，即人脸被逆时针旋转了270度，头顶向右
  ST_FACE_UNKNOWN = 0xf  ///< 人脸方向未知，该参数会严重影响检测性能，占时不支持
} st_face_orientation;

/// @brief  图像格式定义
typedef enum {
  ST_PIX_FMT_GRAY8,    ///< Y    1      	 8bpp ( 单通道8bit灰度像素 )
  ST_PIX_FMT_YUV420P,  ///< YUV  4:2:0   	12bpp ( 3通道, 一个亮度通道,
                       ///< 另两个为U分量和V分量通道, 所有通道都是连续的 )
  ST_PIX_FMT_NV12,  ///< YUV  4:2:0   	12bpp ( 2通道, 一个通道是连续的亮度通道,
                    ///< 另一通道为UV分量交错 )
  ST_PIX_FMT_NV21,  ///< YUV  4:2:0   	12bpp ( 2通道, 一个通道是连续的亮度通道,
                    ///< 另一通道为VU分量交错 )
  ST_PIX_FMT_BGRA8888,  ///< BGRA 8:8:8:8 	32bpp ( 4通道32bit BGRA 像素 )
  ST_PIX_FMT_BGR888,    ///< BGR  8:8:8   	24bpp ( 3通道24bit BGR 像素 )
  ST_PIX_FMT_RGBA8888,  ///< RGBA 8:8:8：8	32bpp ( 4通道32bit RGBA 像素 )
  ST_PIX_FMT_RGB888     ///< RGB  8:8:8		24bpp ( 3通道24bit RGB 像素 )
} st_pixel_format;

/// @brief 特征头定义
typedef struct st_feature_header_t {
  int ver;  ///< 版本信息
  int idx;  ///< 数组下标索引
  int len;  ///< ST_FEATURE全部内容的长度, 包括feature_header和特征数组,
            ///< 按字节计算, 与sizeof(st_feature_header_t)定义不同
} st_feature_header_t;

typedef struct st_image_t {
  unsigned char *data;           ///< 图像数据指针
  st_pixel_format pixel_format;  ///< 像素格式
  int width;                     ///< 宽度
  int height;                    ///< 高度
  int stride;                    ///< 跨度
  long int timestamp;            ///< 时间戳(ms)
} st_image_t;

typedef struct st_detection_result_t {
  st_rect_t rect;   ///< 人脸框
  float score;      ///< 结果分数
  int label;        ///< 结果标签
  int orientation;  ///< 人脸朝向
  unsigned reserved1;
  unsigned reserved2;
} st_detection_result_t;

/// @brief 特征格式定义
typedef struct st_feature_t {
  int ver;  ///< 特征版对应模型本号
  int idx;  ///< 特征索引序号
  int len;  ///< ST_FEATURE全部内容的长度, 包括feature_header和特征数组,
            ///< 按字节计算, 与sizeof(st_feature_header_t)定义不同
  float feat[0];  ///< 特征数组
} st_feature_t;

/// @brief 人脸信息结构体
typedef struct st_face_t {
  st_rect_t rect;  ///< 代表面部的矩形区域
  float
      score;  ///< 置信度，用于筛除负例，与人脸照片质量无关，值越高表示置信度越高。
  int points_count;  ///< 人脸关键点数组的长度，如果没有计算关键点，则为0
  float yaw;       ///< 水平转角，真实度量的左负右正
  float pitch;     ///< 俯仰角，真实度量的上负下正
  float roll;      ///< 旋转角，真实度量的左负右正
  float eye_dist;  ///< 两眼间距
  int occlusion;   ///< 遮挡信息
  int ID;  ///< faceID，用于表示在实时人脸跟踪中的相同人脸在不同帧多次出现，在人脸检测的结果中无实际意义
  st_pointf_t points_array
      [256];  ///< 人脸关键点的数组支持21点，106点，以及为将来预留的空间
  void *extra_data;  ///< 保留数据
} st_face_t;

/// @brief 人脸关键点信息结构体
typedef struct st_landmarks_t {
  st_pointf_t *points_array;  ///< 关键点数组
  unsigned char *occlusion;
  int points_count;  ///< 关键点数量
  int label;         ///< tracking得到
  float score;       ///< 关键点置信的label
  float label_score;
  void *extra_info;  ///< the real type is cv_landmarks_extra_info_t* or others,
                     ///< it's up to the models
  float *points_score;
  unsigned int reserved1;
  unsigned int reserved2;
} st_landmarks_t;

/// @brief 人脸图像质量判断维度
typedef enum {
  FACE_DISTANCE2CENTER = (1 << 2),
  FACE_SIZE = (1 << 3),
  FACE_OCCLUSION = (1 << 4),
  FACE_BRIGHTNESS = (1 << 5),
  FACE_SHARPNESS = (1 << 6),
  FACE_MOUTH = (1 << 7),
  FACE_EYES = (1 << 8),
  FACE_MISSING = (1 << 10),
  FACE_ALL = -1
} st_face_quality_flag;

/// @brief 人脸图像质量判断返回结果
typedef struct st_face_quality_t {
  float
      distance2center;  ///< [0,1]. max(1-五官中心到图像中心距离/图像短边距离,0)
  float size;        ///< [0,1]. 五官在图像中的面积/图像面积
  float occlusion;   ///< [0,1]. 未遮挡的五官面积/五官面积
  float brightness;  ///< [-1,1]. low to high
  float sharpness;   ///< [0,1]. bad to good
  float mouth;       ///< [0,1]. open to close
  float leftEye;     ///< [0,1]. close to open
  float rightEye;    ///< [0,1]. close to open
  float missing;     ///< [0,1]. 五官在图像中的面积/五官面积
} st_face_quality_t;

/// 人脸选帧、检测、跟踪配置选项
#define ST_FACE_DEFAULT_CONFIG 0x00000000  ///< 默认选项，不设置任何开关
#define ST_FACE_RESIZE_IMG_320W 0x00000002  ///< resize图像为长边320的图像
#define ST_FACE_RESIZE_IMG_640W 0x00000004  ///< resize图像为长边640的图像
#define ST_FACE_RESIZE_IMG_1280W 0x00000008  ///< resize图像为长边1280的图像
#define ST_FACE_RESIZE_IMG_640x400 \
  0x00000010  ///< resize图像长宽为640x400的图像
#define ST_FACE_RESIZE_IMG_640x480 \
  0x00000020  ///< resize图像长宽为640x480的图像

#define ST_FEATURE_HEADER(pf) ((st_feature_header_t *)(pf))
/// @brief 特征结构体长度
#define ST_FEATURE_SIZE(pf) (ST_FEATURE_HEADER(pf)->len)
/// @brief 特征结构体中特征长度
#define ST_FEATURE_LENGTH(pf) \
  ((ST_FEATURE_HEADER(pf)->len - sizeof(st_feature_header_t)) / sizeof(float))
/// @brief 特征序列换为字符串需要分配的空间字节数
#define ST_ENCODE_FEATURE_SIZE(pf) \
  ((ST_FEATURE_HEADER(pf)->len + 2) / 3 * 4 + 1)

// /// @brief 创建图像指针并分配内存
// /// @param[in] width 输入宽度
// /// @param[in] height 输入高度
// /// @param[in] pixel_format 输入像素格式
// /// @param[out] image 输出图像指针所在地址
// /// @return 成功返回ST_OK, 否则返回错误码
// ST_SDK_API
// st_result_t
// st_image_allocate(int width, int height, st_pixel_format pixel_format,
// st_image_t **image);

// /// @brief 释放图像数据
// /// @param[in]  image 图像指针
// ST_SDK_API
// void
// st_image_release(st_image_t *image);

#endif
