#ifndef ST_FACE_UNLOCK_H_
#define ST_FACE_UNLOCK_H_
#include "st_common.h"

/// @brief 获取人脸验证模型的版本号
/// @param[in]	verify_handle 已初始化的人脸验证句柄
/// @param[out]	version_num 返回模型版本号
/// @return 返回ST_OK 失败返回错误码
ST_SDK_API
st_result_t st_face_unlock_verify_get_model_version(int *version_num);

/// @brief
/// 提取人脸特征，返回特征数据长度，可以把返回数组编码成字符串后存储起来以便以后使用
/// @param[in]	verify_handle	已初始化的人脸验证句柄
/// @param[in]	image 用于检测的图像数据
/// @param[in]	piexl_format 用于检测的图像数据的像素格式,推荐BGR888格式
/// @param[in]	image_width 用于检测的图像的宽度(以像素为单位)
/// @param[in]	image_height 用于检测的图像的高度(以像素为单位)
/// @param[in]	image_stride
/// 用于检测的图像中每一行的跨度(以像素为单位)，图像跨度=图像宽度*图像维度
/// @param[in]	p_face 输入待处理的人脸信息，需要包括关键点信息，即
/// p_face.points_array 不为NULL
/// @param[out]	p_feature
/// 人脸特征数据，需要调用st_verify_release_feature函数释放
/// @param[out]	feature_blob_size
/// 特征的长度，包含st_feature_header_t的长度，与st_feature_header_t->len一致
/// @return 成功返回ST_OK，否则返回错误类型
ST_SDK_API
st_result_t st_face_unlock_verify_get_feature(
    const unsigned char *image, st_pixel_format pixel_format, int image_width,
    int image_height, int image_stride, const st_face_t *p_face,
    st_feature_t **p_feature, unsigned int *feature_blob_size);

/// @brief	特征信息编码成明文字符串，编码后的字符串用于保存
/// @param[in]	feature 输入的特征信息
/// @param[out]	feature_str 输出的编码后的字符串,由用户分配和释放
/// @return 成功返回ST_OK，否则返回错误类型
ST_SDK_API
st_result_t st_face_unlock_verify_serialize_feature(const st_feature_t *feature,
                                                    char *feature_str);

/// @brief	解码字符串成特征信息
/// @param[in]	feature_str 输入的待解码的字符串
/// @param[out] 返回解码后的feature, 需要用户使用
/// st_face_unlock_verify_release_feature() 释放
/// @return 成功返回ST_OK，否则返回错误类型
ST_SDK_API
st_result_t st_face_unlock_verify_deserialize_feature(const char *feature_str,
                                                      st_feature_t **feature);

/// @brief	人脸验证
/// @param[in]	verify_handle	已初始化的人脸验证句柄
/// @param[in]	feature1	第一张人脸特征信息
/// @param[in]	feature2	第二张人脸特征信息
/// @param[out]	score	人脸验证相似度得分，范围0-1，得分越接近1越相似
/// @return 成功返回ST_OK，否则返回错误类型
ST_SDK_API
st_result_t st_face_unlock_verify_compare_feature(const st_feature_t *feature1,
                                                  const st_feature_t *feature2,
                                                  float *score);

/// @brief	释放提取人脸特征时分配的空间
/// @param[in]  feature 提取到的人脸特征信息
ST_SDK_API
void st_face_unlock_verify_release_feature(st_feature_t *feature);

/// @brief 输入一帧图片,进行防hack检测
/// @param[in] 	hackness_binocular_handle 已初始化的句柄
/// @param[in]	image 用于检测的图像数据
/// @param[in]	piexl_format 用于检测的图像数据的像素格式,推荐BGR888格式
/// @param[in]	image_width 用于检测的图像的宽度(以像素为单位)
/// @param[in]	image_height 用于检测的图像的高度(以像素为单位)
/// @param[in]	image_stride
/// 用于检测的图像中每一行的跨度(以像素为单位)，图像跨度=图像宽度*图像维度
/// @param[in]	face_info 人脸信息
/// @param[in]	orientation 图片中人脸的方向
/// @param[out] score 返回该帧得分,分数越大越可能是hack
/// @return 成功返回ST_OK, 否则返回错误码
ST_SDK_API
st_result_t st_face_unlock_hackness_binocular_detect(
    const unsigned char *image_first, st_pixel_format pixel_format_first,
    int image_width_first, int image_heigh_first, int image_stride_first,
    const st_face_t *face_info_first, st_face_orientation orientation_first,
    const unsigned char *image_second, st_pixel_format pixel_format_second,
    int image_width_second, int image_heigh_second, int image_stride_second,
    const st_face_t *face_info_second, st_face_orientation orientation_second,
    float *score);

#endif