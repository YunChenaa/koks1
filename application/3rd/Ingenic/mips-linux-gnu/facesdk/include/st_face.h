#ifndef ST_FACE_API_H_
#define ST_FACE_API_H_
#include "st_common.h"

/// @brief  输出当前SDK所支持的算法及内置模型信息
ST_SDK_API
void st_face_algorithm_info();

/// @brief  带logSDK打开log开关
ST_SDK_API void st_face_open_log();

/// @brief  加载license文件，该函数只应该成功调用一次
/// param[in]	szLicense 授权文件中的字符串，license文件的全部内容
ST_SDK_API
st_result_t st_face_init_license(const char *szLicense);

/// @brief  返回当前sdk的版本号
ST_SDK_API
const char *st_face_get_version();

/// @brief	获取人脸检测的阀值
/// @param[in]	handle 人脸检测句柄
/// @param[out]	threshold	人脸检测的阀值，如果未设置阈值，则返回默认阈值
ST_SDK_API
st_result_t st_face_detect_get_threshold(float *threshold);

/// @brief
/// 设置当前人脸检测的阀值，需要在调用st_face_detect之前调用此函数，阈值需大于0，设置后调用st_face_detect_get_threshold为新设置后的值)
/// @param[in]	detector_handle 人脸检测句柄
/// @param[in]	threshold 人脸检测新阀值
ST_SDK_API
st_result_t st_face_detect_set_threshold(float threshold);

/// @brief	人脸检测
/// @param[in]	image 用于检测的图像数据
/// @param[in]	piexl_format 用于检测的图像数据的像素格式,推荐BGR888格式
/// @param[in]	image_width 用于检测的图像的宽度(以像素为单位)
/// @param[in]	image_height 用于检测的图像的高度(以像素为单位)
/// @param[in]	image_stride 用于检测的图像中每一行的跨度(以像素为单位)
/// 图像跨度=图像宽度*图像维度
/// @param[in]	orientation 图片中人脸的方向
/// @param[out]	p_faces_array
/// 检测到的人脸信息数组，api负责分配内存，需要调用st_face_release_detector_result函数释放
/// @param[out]	p_faces_count 检测到的人脸数量
/// @return 成功返回ST_OK，否则返回错误类型
ST_SDK_API
st_result_t st_face_detect(const unsigned char *image,
                           st_pixel_format pixel_format, int image_width,
                           int image_height, int image_stride,
                           st_face_orientation orientation,
                           st_face_t **p_faces_array, int *p_faces_count);

/// @brief	释放人脸检测返回结果时分配的空间
/// @param[in]	faces_array 检测到的人脸信息数组
ST_SDK_API
void st_face_release_detector_result(st_face_t *faces_array);

/// @brief 睁闭眼检测
/// @param[in]	handle 已初始化的睁闭眼检测句柄
/// @param[in]	image 用于检测的图像数据
/// @param[in]	piexl_format 用于检测的图像数据的像素格式
/// @param[in]	image_width 用于检测的图像的宽度(以像素为单位)
/// @param[in]	image_height 用于检测的图像的高度(以像素为单位)
/// @param[in]	image_stride 用于检测的图像中每一行的跨度(以像素为单位)
/// 图像跨度=图像宽度*图像维度
/// @param[in]	lt 图片中人脸的位置的矩形框，可由detect得到
/// @param[in]	orientation 图片中人脸的方向
/// @param[out]	score
/// 包含左右眼的信息score[0]为左眼分数，score[1]为右眼分数,-1闭眼，1睁眼
/// @return 成功返回ST_OK，否则返回错误类型
ST_SDK_API
st_result_t st_face_ocular_detect_with_rect(
    const unsigned char *image, st_pixel_format pixel_format, int image_width,
    int image_height, int image_stride, const st_rect_t face_rect,
    st_face_orientation orientation, float *score);

/// @brief 睁闭眼检测
/// @param[in]	handle 已初始化的睁闭眼检测句柄
/// @param[in]	val 新的睁闭眼阈值[0,10]
/// @param[in]	real_val 设置的实际值
/// @return 成功返回ST_OK，否则返回错误类型
ST_SDK_API
st_result_t st_face_ocular_set_unlock_open_threshold(float val,
                                                     float *real_val);

/// @brief 睁闭眼检测
/// @param[in]	handle 已初始化的睁闭眼检测句柄
/// @param[in]	val 新的睁闭眼眼睛有效阈值[0,10]
/// @param[in]	real_val 设置的实际值
/// @return 成功返回ST_OK，否则返回错误类型
ST_SDK_API
st_result_t st_face_ocular_set_unlock_valid_threshold(float val,
                                                      float *real_val);

/// @brief 睁闭眼检测
/// @param[in]	handle 已初始化的睁闭眼检测句柄
/// @param[in]	val 新的睁闭眼眼睛有效阈值[0,5]
/// @param[in]	real_val 设置的实际值
/// @return 成功返回ST_OK，否则返回错误类型
ST_SDK_API
st_result_t st_face_ocular_set_unlock_frame_size(int val, int *real_val);

/// @brief
/// 创建实时人脸跟踪句柄。可以调用st_face_track_set_detect_face_cnt_limit设置检测的最大人脸数目，默认为不限制人脸检测数量。
/// @param[in]	model_path 模型文件的绝对路径或相对路径，若不指定模型可为NULL
/// @param[in]	config 配置参数，保留，传入0即可
/// @param[out]	handle	函数成功时被设置为有效的tracker handle
/// @return 返回ST_OK 失败返回错误码
ST_SDK_API
st_result_t st_face_create_tracker(const char *model_path, unsigned int config,
                                   st_handle_t *handle);

/// @brief	销毁已初始化的实时人脸跟踪句柄
/// @param[in]	tracker_handle 已初始化的实时人脸跟踪句柄
ST_SDK_API
void st_face_destroy_tracker(st_handle_t tracker_handle);

/// @brief	对连续视频帧进行实时快速人脸跟踪
/// @param[in]	tracker_handle	已初始化的实时人脸跟踪句柄
/// @param[in]	image 用于检测的图像数据
/// @param[in]	piexl_format 用于检测的图像数据的像素格式,推荐BGR888格式
/// @param[in]	image_width 用于检测的图像的宽度(以像素为单位)
/// @param[in]	image_height 用于检测的图像的高度(以像素为单位)
/// @param[in]	image_stride
/// 用于检测的图像中每一行的跨度(以像素为单位)，图像跨度=图像宽度*图像维度
/// @param[in]	orientation 视频中人脸的方向
/// @param[out]	p_faces_array
/// 检测到的人脸信息数组，api负责分配内存，需要调用st_facesdk_release_tracker_result函数释放
/// @param[out]	p_faces_count 检测到的人脸数量
/// @return 	成功返回ST_OK，否则返回错误类型
ST_SDK_API
st_result_t st_face_track(st_handle_t tracker_handle,
                          const unsigned char *image,
                          st_pixel_format pixel_format, int image_width,
                          int image_height, int image_stride,
                          st_face_orientation orientation,
                          st_face_t **p_faces_array, int *p_faces_count);

/// @brief	重置人脸跟踪检测器
/// @param[in]	tracker_handle 已初始化的实时人脸跟踪句柄
ST_SDK_API
void st_face_reset_tracker(st_handle_t tracker_handle);

/// @brief	释放实时人脸跟踪返回结果时分配的空间
/// @param[in]	faces_array 检测到的人脸信息数组
/// @param[in]	faces_count 检测到的人脸数量
ST_SDK_API
void st_face_release_tracker_result(st_face_t *faces_array, int faces_count);

/// @brief
/// 设置检测到的最大人脸数目N，持续track已检测到的N个人脸直到人脸数小于N再继续做detect,如果未调用则不对检测人脸数量做限制
/// @param[in]	tracker_handle 已初始化的实时人脸跟踪句柄
/// @param[in]	detect_face_cnt_limit 最大人脸数目N，-1表示不设上限
/// @param[out]	val 如果为非空指针，则返回采用的新的值，注意，由于内部的限制，
/// 实际采用的值并不一定等于detect_face_cnt_limit
/// @return 成功返回ST_OK，否则返回错误类型
/// (实际设置检测到的最大目标数目N可能小于detect_face_cnt_limit)
ST_SDK_API
st_result_t st_face_track_set_detect_face_cnt_limit(st_handle_t tracker_handle,
                                                    int detect_face_cnt_limit,
                                                    int *val);

ST_SDK_API
int st_face_quality_assessment(const st_image_t *image,
                               const st_landmarks_t *landmarks,
                               st_face_quality_flag flag,
                               st_face_quality_t *result);

ST_SDK_API
st_result_t st_face_blur_detect(st_image_t *src, st_rect_t rect,
                                double *sharpness);

ST_SDK_API
st_result_t st_face_pose_detect(const st_pointf_t *points, int point_count,
                                float *yaw, float *pitch, float *roll);

ST_SDK_API
st_result_t st_face_resize_image(st_image_t *src, st_image_t *dst);

ST_SDK_API
st_result_t st_face_convert_image(st_image_t *src, st_image_t *dst);
#endif