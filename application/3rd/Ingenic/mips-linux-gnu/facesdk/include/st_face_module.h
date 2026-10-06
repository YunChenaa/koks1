#ifndef ST_FACE_MODULE_H_
#define ST_FACE_MODULE_H_

#include <vector>

#include "st_common.h"

#define ST_FACE_MODULE_STATUS_DETECT_FAILED 1  ///< 人脸检测失败，没有人脸
#define ST_FACE_MODULE_STATUS_VERIFY_FAILED \
  2  ///< 比对失败，如果在录入阶段，当前图像会与录入的前一张图像做比对
#define ST_FACE_MODULE_STATUS_HACKNESS_FAILED 3     ///< 活体检测失败
#define ST_FACE_MODULE_STATUS_QUALITY_FAILED 4      ///< 质量检测失败
#define ST_FACE_MODULE_STATUS_GET_FEATURE_FAILED 5  ///< 提取人脸特征失败
#define ST_FACE_MODULE_STATUS_FACE_UP 6             ///< 人脸偏上
#define ST_FACE_MODULE_STATUS_FACE_DOWN 7           ///< 人脸偏下
#define ST_FACE_MODULE_STATUS_FACE_LEFT 8           ///< 人脸偏左
#define ST_FACE_MODULE_STATUS_FACE_RIGHT 9          ///< 人脸偏右
#define ST_FACE_MODULE_STATUS_ABORTED 10            ///< 算法中止
#define ST_FACE_MODULE_STATUS_FACE_ENROLLED 11      ///< 此用户已录入
#define ST_FACE_MODULE_STATUS_NO_SUCH_USER 12    ///< 此用户未录入或者已被删除
#define ST_FACE_MODULE_STATUS_FACE_TOO_CLOSE 13  ///< 人脸离镜头太近
#define ST_FACE_MODULE_STATUS_FACE_TOO_FAR 14    ///< 人脸离镜头太远
#define ST_FACE_MODULE_STATUS_LEFT_EYEBROW_OCCLUSION 15   ///< 左眉遮挡
#define ST_FACE_MODULE_STATUS_RIGHT_EYEBROW_OCCLUSION 16  ///< 右眉遮挡
#define ST_FACE_MODULE_STATUS_LEFT_EYE_OCCLUSION 17       ///< 左眼遮挡
#define ST_FACE_MODULE_STATUS_RIGHT_EYE_OCCLUSION 18      ///< 右眼遮挡
#define ST_FACE_MODULE_STATUS_FACE_OCCLUSION 19           ///< 脸部遮挡
#define ST_FACE_MODULE_STATUS_FACE_IOU_ERROR 20           ///< iou过低
#define ST_FACE_MODULE_STATUS_MORE_HACKNESS_FAILED \
  21  ///< 假体次数比活体次数多了5次
#define ST_FACE_MODULE_STATUS_BOTH_EYE_OCCLUSION 22  ///< 两只眼睛均被遮挡
#define ST_FACE_MODULE_STATUS_FACE_LICENSE_ERROR 23  ///< license还未初始化成功
#define ST_FACE_MODULE_STATUS_FACE_PARAMETER_ERROR \
  24                                         ///< 参数错误或者超过合法范围
#define ST_FACE_MODULE_STATUS_POSE_ERROR 25  ///< 人脸姿态不符合要求
#define ST_FACE_MODULE_STATUS_FAKE_HACKNESS_FAILED \
  26  ///< 活体算法判断为真人， 但是需要当做假体对待
#define ST_FACE_MODULE_STATUS_ALIGN_FAILED 27  ///< align失败
#define ST_FACE_MODULE_STATUS_BLUR_FAILED 28  ///< 人脸区域过于模糊， 不符合标准

/// enroll
#define ST_FACE_MODULE_STATUS_ENROLL_OK 100             ///< 录入成功
#define ST_FACE_MODULE_STATUS_CREATE_FOLDER_FAILED 101  ///< 创建模板目录失败
#define ST_FACE_MODULE_STATUS_SAVE_TEMPLATE_FAILED 102  ///< 保存模板失败
#define ST_FACE_MODULE_STATUS_FACE_ADDED 103  ///< 录入阶段，人脸已添加
#define ST_FACE_MODULE_STATUS_FACE_DIRECTION_MIDDLE 104  ///< 录入了正面人脸
#define ST_FACE_MODULE_STATUS_FACE_DIRECTION_UP 105      ///< 录入了朝上人脸
#define ST_FACE_MODULE_STATUS_FACE_DIRECTION_DOWN 106    ///< 录入了朝下人脸
#define ST_FACE_MODULE_STATUS_FACE_DIRECTION_LEFT 107    ///< 录入了朝左人脸
#define ST_FACE_MODULE_STATUS_FACE_DIRECTION_RIGTH 108   ///< 录入了朝右人脸
#define ST_FACE_MODULE_STATUS_MAXIMUM_USER_NUMBER \
  109  ///< 达到了录入的最大用户数
#define ST_FACE_MODULE_STATUS_DIRECTION_ERROR \
  110  ///< 在录入的ST_MODULE_ENROLL_MODE_INTERACTIVE模式下，此状态表示人脸方向与设定录入的方向不一致
       ///< 在录入的ST_MODULE_ENROLL_MODE_NORMAL模式下，此状态表示人脸方向不是正向

/// unlock
#define ST_FACE_MODULE_STATUS_UNLOCK_OK 200               ///< 解锁成功
#define ST_FACE_MODULE_STATUS_UNLOCK_FAILED 201           ///< 解锁失败
#define ST_FACE_MODULE_STATUS_NO_TEMPLATE 202             ///< 解锁时没有模板
#define ST_FACE_MODULE_STATUS_TEMPLDATE_FILE_DAMAGED 203  ///< 模板文件损坏
#define ST_FACE_MODULE_STATUS_UNLOCK_WITH_EYES_CLOSE \
  204  ///< 解锁成功但是双眼闭
#define ST_FACE_MODULE_STATUS_UNLOCK_WITH_TAG_USER \
  205  ///< 检测到非注册的自定义标签人员
#define ST_FACE_MODULE_STATUS_UNLOCK_WITHOUT_SMILE \
  206  ///< 解锁成功,但没有微笑， 按失败处理
#define ST_FACE_MODULE_STATUS_UNLOCK_WITH_MOUTH_OPEN \
  207  ///< 解锁成功但嘴巴张开

/// factory test
#define ST_FACE_MODULE_STATUS_FACTORY_OK 300         ///< 工厂模型测试通过
#define ST_FACE_MODULE_STATUS_RECT_MATCH_FAILED 301  ///< 人脸框比对不通过
#define ST_FACE_MODULE_STATUS_DETECT_SCORE_MATCH_FAILED \
  302  ///< 人脸检测分数比对不通过
#define ST_FACE_MODULE_STATUS_LANDMARKS_MATCH_FAILED 303  ///< 关键点比对不通过
#define ST_FACE_MODULE_STATUS_OCCLUSION_MATCH_FAILED 304  ///< 遮挡比对不通过
#define ST_FACE_MODULE_STATUS_ALIGN_SCORE_MATCH_FAILED \
  305  ///< Align分数比对不通过
#define ST_FACE_MODULE_STATUS_FEATURE_MATCH_FAILED \
  306  ///< 特征字符串比对不通过
#define ST_FACE_MODULE_STATUS_LIVENESS_SCORE_MATCH_FAILED \
  307  ///< 活体分数比对不通过
#define ST_FACE_MODULE_STATUS_EYE_STATE_DETECT_FAILED 308  ///< 睁闭眼检测失败
#define ST_FACE_MODULE_STATUS_EYE_SCORE_MATCH_FAILED \
  309  ///< 睁闭眼检测分数比对不通过

#define ST_FACE_MODULE_STATUS_UNFINISHED 310 //人脸录入未完成，继续调用录入接口, 重点新增

// enroll direction
#define ST_MODULE_DIRECTION_UP 0x0010      ///< 人脸朝上
#define ST_MODULE_DIRECTION_DOWN 0x0008    ///< 人脸朝下
#define ST_MODULE_DIRECTION_LEFT 0x0004    ///< 人脸朝左
#define ST_MODULE_DIRECTION_RIGHT 0x0002   ///< 人脸朝右
#define ST_MODULE_DIRECTION_MIDDLE 0x0001  ///< 人脸正向

/// 人脸信息
typedef struct st_module_face_info_t {
  int left;     ///< 人脸矩形框最左边的坐标
  int top;      ///< 人脸矩形框最上边的坐标
  int right;    ///< 人脸矩形框最右边的坐标
  int bottom;   ///< 人脸矩形框最下边的坐标
  float yaw;    ///< 围绕Y轴旋转，也叫偏航角
  float pitch;  ///< 围绕X轴旋转，也叫做俯仰角
  float roll;   ///< 围绕Z轴旋转，也叫翻滚角
} st_module_face_info_t;

/// @brief 人脸姿态回调函数
/// @param[in]  face_info 人脸信息
typedef void (*callback_calc_pose)(st_module_face_info_t *face_info);

/// @brief 人脸数据和人脸框回调函数
/// @param[in]  image 人脸数据
/// @param[in]  rect 人脸框
typedef int (*callback_face_rect)(st_image_t *image,
                                  const std::vector<st_rect_t> *rect);

/// @brief 算法终端回调，返回`1`表示算法中止，否则算法继续执行
typedef int (*callback_algo_abort)();

/// @brief 获取软光敏状态，是否为白天 true表示白天，false表示夜晚
typedef bool (*callback_day_mode)();

/// 录入模式定义
typedef enum st_module_enroll_mode {
  ST_MODULE_ENROLL_MODE_NORMAL,  ///< 正常模式，按照用户指定的张数进行录入
  ST_MODULE_ENROLL_MODE_INTERACTIVE,  ///< 交互模式，需要用户根据提示分别录入上、下、左、右和正脸五个方向
} st_module_enroll_mode;

/// @brief 数据AES加解密回调函数
/// @param[in]  in   需要加解密的数据
/// @param[in]  len  需要加解密的数据的长度
/// @param[in]  out  加解密之后的数据
/// @param[in]  key  16字节的密钥
/// @param[in]  mode 0为加密模式/1为解密模式
typedef int (*callback_aes_crypto)(unsigned char *in, int len,
                                   unsigned char *out, char *key, int mode);
typedef void (*callback_log)(const char *msg);
/// 解锁模式定义
typedef enum st_module_unlock_mode {
  ST_MODULE_UNLOCK_MODE_NORMAL,  ///< 正常模式
  ST_MODULE_UNLOCK_MODE_DEMO     ///< 演示模式，不进行比对验证
} st_module_unlock_mode;

/// 图像定义
/// NOTE: 特别注意,V3A中第一张图传入IR-GRAY8，第二张图传入RGB-NV12.
typedef struct st_module_image_t {
  unsigned char *first_image;           ///< 第一张图像数据指针
  unsigned char *second_image;          ///< 第二张图像数据指针
  st_pixel_format first_pixel_format;   ///< 像素格式
  st_pixel_format second_pixel_format;  ///< 像素格式
  st_face_orientation orientation;      ///< 人脸方向
  int width;                            ///< 宽度(以像素为单位)
  int height;                           ///< 高度(以像素为单位)
  int first_stride;                     ///< 跨度, 即每行所占的字节数
  int second_stride;                    ///< 跨度, 即每行所占的字节数
} st_module_image_t;

/// 句柄配置参数
typedef struct st_module_config_t {
  char *template_save_path;  ///< 模板存储路径
  int resize_image;  ///< 图像resize参数，可选参数ST_FACE_DEFAULT_CONFIG和ST_FACE_RESIZE_IMG_XXX，定义在st_commom.h文件中
  int liveness_LOGIc_method;       ///< 算法逻辑优化策略
  bool load_verify;                /// load verify model at init?
  char *aes_enc_key;               ///< aes 加密key
  callback_calc_pose calc_pose;    ///< 设置人脸姿态回调函数，可以为NULL
  callback_face_rect face_rect;    ///< 设置人脸框回调函数，可以为NULL
  callback_algo_abort algo_abort;  ///< 算法中断回调，可以为NULL
  callback_aes_crypto aes_crypto;  ///< 设置AES加解密的回调函数，不可以为NULL
  callback_log x_log;  ///< 设置日志回调函数，可以为NULL
  callback_day_mode is_daymode;
} st_module_config_t;

/// 内部参数配置
typedef struct st_module_param_config_t {
  // argorithm threshold
  float config_detect_threshold;         ///< default 0.27: 0~1.0
  float config_align_threshold;          ///< default 1.0f: 0~10.0
  float config_hackness_threshold;       ///< default 0.98f
  float config_verify_threshold;         ///< default 0.7f
  float config_verify_rgb_threshold;     ///< default 0.55f
  float config_enroll_verify_threshold;  ///< default 0.5f
  float config_quality_brightness_max;   ///< default 0.5f
  float config_quality_brightness_min;   ///< default -0.5f
  float config_sharpness_threshold;      ///< default 0.83f
  // face direction parameters
  float config_face_middle_max;      ///< yaw pitch roll default 10
  float config_face_middle_min;      ///< yaw pitch roll default -10
  float config_face_middle_yaw_max;  ///< yaw default 10
  float config_face_middle_yaw_min;  ///< yaw default -10
  float config_face_left_max;        ///< yaw default -10
  float config_face_left_min;        ///< yaw default -45
  float config_face_right_max;       ///< yaw default 45
  float config_face_right_min;       ///< yaw default 10
  float config_face_up_max;          ///< pitch default -10
  float config_face_up_min;          ///< pitch default -45
  float config_face_down_max;        ///< pitch default 45
  float config_face_down_min;        ///< pitch default 10
  float config_face_roll_max;        ///< roll default 20
  float config_face_roll_min;        ///< roll default -20
  float config_face_up_padding;      ///< default 6.0
  float config_face_down_padding;    ///< default 3.0
  float config_face_up_range;        ///< default 20
  float config_face_down_range;      ///< default 20
  float config_face_middle_range;    ///< default 20
  // face distance parameters
  float config_face_close_threshold;  ///< greater than
                                      ///< (face_rect.right-face_rect.left)
  float config_face_far_threshold;    ///< less than
                                      ///< (face_rect.right-face_rect.left)
  float config_face_iou_threshold;    ///< default 0.6
  // face occlusion parameters
  int config_left_eye_occlusion_max_count;      ///< value in 1~10, 11 or bigger
                                                ///< will override this feature
  int config_right_eye_occlusion_max_count;     ///< value in 1~10, 11 or bigger
                                                ///< will override this feature
  int config_left_eyebrow_occlusion_max_count;  ///< value in 1~9, 10 or bigger
                                                ///< will override this feature
  int config_right_eyebrow_occlusion_max_count;  ///< value in 1~9, 10 or bigger
                                                 ///< will override this feature
  int config_face_occlusion_max_count;  ///< value in 1~66, 67 or bigger will
                                        ///< override this feature

  int config_enable_enroll_hackness;  ///< default 31 means enable hackness in
                                      ///< all direction,reference `enroll
                                      ///< direction`

  float config_spot_detect_threshold;  ///< value in 0.0~1.0, default 0.2

  int config_verify_yaw_min;    ///< default -20
  int config_verify_yaw_max;    ///< default 20
  int config_verify_roll_min;   ///< default -25
  int config_verify_roll_max;   ///< default 25
  int config_verify_pitch_min;  ///< default -35
  int config_verify_pitch_max;  ///< default 40

  int keypointStabilizer_win_size;
  int keypointStabilizer_thresh;
} st_module_param_config_t;

typedef struct st_module_algo_info {
  st_rect_t face_rect;
  int occlusion_points[ST_FACE_LANDMARK_POINTS_CNT];
  float iou_score;
  float quality_score;
  float verify_score;
  int verify_id;
  float liveness_score;
  int status;
} st_module_algo_info;

typedef struct st_point3f_t {
  float x;  ///< 点的水平方向坐标, 为浮点数
  float y;  ///< 点的竖直方向坐标, 为浮点数
  float z;  ///< 点的深度坐标, 为浮点数
} st_point3f_t;

/// @brief
/// 加载license文件，并通过atsha204a加密芯片进行验证，该函数只应该成功调用一次
/// param[in]	szLicense 授权文件中的字符串，license文件的全部内容
ST_SDK_API
st_result_t st_face_init_license_atsha204a(const char *szLicense);

/// @brief 初始化人脸认证资源
/// @param[in]  config 配置参数
ST_SDK_API
st_result_t st_face_module_init(const st_module_config_t *config);

typedef struct st_module_face_attribute_t {
  int gender;   ///< 检测的性别
  int emotion;  ///< 人脸当前表情
  float age;    ///< 人脸检测的年龄
} st_module_face_attribute_t;

/// @brief 设置内部各项参数
/// @param[in]  config 配置参数
ST_SDK_API
st_result_t st_face_module_set_param_config(
    const st_module_param_config_t *config);

/// @brief 设置逻辑层对活体的策略方案
/// @param[in]  value 设置的方案
ST_SDK_API
st_result_t st_face_module_set_liveness_LOGIc_method(int value);

/// @brief 设置录入模式
/// @param[in]  mode 录入模式
ST_SDK_API
st_result_t st_face_module_set_enroll_mode(st_module_enroll_mode mode);

/// @brief
/// 设置单用户录入图片数，默认为1，此接口仅在ST_MODULE_ENROLL_MODE_NORMAL模式下有效
/// @param[in]  num
/// 单个用户录入图片数，仅在ST_MODULE_ENROLL_MODE_NORMAL模式下有效
ST_SDK_API
st_result_t st_face_module_set_enroll_number(int num);

/// @brief
/// 设置下一帧录入的方向，默认为ST_MODULE_DIRECTION_MIDDLE，此接口仅在ST_MODULE_ENROLL_MODE_INTERACTIVE模式下有效
/// @param[in]  direction 待录入的方向
ST_SDK_API
st_result_t st_face_module_set_enroll_direction(int direction);

/// @brief 设置解锁模式
/// @param[in]  mode 解锁模式
ST_SDK_API
st_result_t st_face_module_set_unlock_mode(st_module_unlock_mode mode);

/// @brief 人脸录入
/// @param[in]  p_image 人脸图像数据
/// @param[out] status 人脸录入状态返回
/// @param[out] user_id
/// 录入的用户id，当status为ST_MODULE_STATUS_ENROLL_OK时有效，否则为-1
ST_SDK_API
st_result_t st_face_module_enroll(const st_module_image_t *p_image, int *status,
                                  int *user_id, unsigned char *occlusion_ratio,bool first_enroll);

/// @brief 特征入库
/// @param[in] aes_feature aes加密特征字符数组指针
/// @param[in] size aes_feature字符数组长度
/// @param[out] status 人脸解锁状态返回
/// @param[out] user_id 人脸id
ST_SDK_API
st_result_t st_face_module_feature_enroll(const char *aes_feature, int size,
                                          int *status, int *user_id);

/// @brief 人脸解锁
/// @param[in]  p_image 人脸图像数据
/// @param[out] status 人脸解锁状态返回
/// @param[out] user_id
/// 解锁的用户id，当status为ST_MODULE_STATUS_UNLOCK_OK时有效，否则为-1
/// @param[out] occlusion 人脸遮挡比例，最小遮挡比例为0，最大遮挡比例为100
ST_SDK_API
st_result_t st_face_module_unlock(const st_module_image_t *p_image, int *status,
                                  int *user_id, unsigned char *occlusion_ratio);

/// @brief 被动加载加载模型
ST_SDK_API
st_result_t st_face_module_load_model(void *addr);

/// @brief 压缩SDK内部内存分配
ST_SDK_API
st_result_t st_face_module_memory_compresssion();

/// @brief 在交互模式下，获取当前录入状态
/// @param[out] status
/// 人脸录入状态返回，status的低5位从高到低分别表示人脸的上、下、左、右和正向
ST_SDK_API
st_result_t st_face_module_get_enroll_status(int *status);

/// @brief 获取前一帧算法执行信息
/// @param[out] info 返回的算法执行信息
ST_SDK_API
st_result_t st_face_module_get_algo_info(st_module_algo_info *info);

/// @brief 设置是否进行活体检测，0为关闭活体检测，否则为打开
ST_SDK_API
st_result_t st_face_module_set_enable_liveness(int enable);

/// @brief 设置是否进行人脸比对，0为关闭人脸比对，否则为打开
ST_SDK_API
st_result_t st_face_module_set_enable_verify(int enable);

/// @brief 设置是否进行人脸模糊判断，0为关闭人脸模糊判断，否则为打开
ST_SDK_API
st_result_t st_face_module_set_enable_blur(int enable);

/// @brief 设置是否进行将RGB注册的人脸用ir解锁图替换，0为不替换，否则为替换
ST_SDK_API
st_result_t st_face_module_set_enable_replace_rgb_feature_by_ir(int enable);

/// @brief 设置是否进行质量检测，0为关闭质量检测，否则为打开，default 1
ST_SDK_API
st_result_t st_face_module_set_enable_quality(int enable);

/// @brief 设置是否允许用户重复录入, 0为不允许重复录入，否则为允许，default 1
ST_SDK_API
st_result_t st_face_module_set_enable_dup_enroll(int enable);

/// @brief 设置是否打开大胡子人脸录入和识别, 0为不打开，否则为打开，default 1
ST_SDK_API
st_result_t st_face_module_set_enable_full_beard(int enable);

/// @brief 设置最大录入的数量，如果不调用，则默认20人
ST_SDK_API
st_result_t st_face_module_set_max_user_count(int count);

/// @breif delete buffer from st_face_module_get_jpeg_from_gray8 and
/// st_face_module_get_gray8_from_jpeg
ST_SDK_API
st_result_t st_face_module_delete_data(unsigned char *data);

/// @brief 获取sdk版本号
/// @return 返回版本号字符串
ST_SDK_API
st_result_t st_face_module_get_version(unsigned char *version);

#define MODEL_NAME_DETECT "DETECT"
#define MODEL_NAME_ALIGN "ALIGN"
#define MODEL_NAME_LIVENESS "LIVENESS"
#define MODEL_NAME_VERIFY "VERIFY"

typedef struct st_face_model_info {
  char w_name[16];              // model's weight binary file name, get from sdk
  char code_name[16];           // model's code binary file name, , get from sdk
  unsigned char *weight_point;  // model's allocated weight buffer addr by sdk
  unsigned int weight_size;     // model's weight size addr
  unsigned int *code_point;     // model's allocated code buffer addr by sdk
  unsigned int code_size;       // model's code size addr
} st_face_model_info;

/// @brief: get model's info
/// @param[in]   model: model's name, MODEL_NAME_XXX
/// @param[out] model_info: model information
ST_SDK_API
st_result_t st_face_module_get_model_info(const char *model,
                                          st_face_model_info *model_info);

/// @brief: update model, user help load model from flash into ddr then call
/// this interface to update
/// @param[in]   model: model's name, MODEL_NAME_XXX
/// @param[in]   model_info: model information
ST_SDK_API
st_result_t st_face_module_update_model(const char *model,
                                        st_face_model_info model_info);

//>> CONFIG_ST_FACEUNLOCK_ACQUISITION

/// @defgroup st_face_module_set_liveness_model st_face_module_acquisition
/// @brief for data acquisition
///
/// @{

/// @brief 人脸信息
typedef struct st_face_image_info {
  st_rect_t rect;
  st_pointf_t *points;
  st_rect_t rect2;
  st_pointf_t *points2;
  int points_count;
  float liveness_score;
  float quality_score;
  float iou_score;
} st_face_image_info;

ST_SDK_API
st_result_t st_face_module_release_image_info(st_face_image_info *image_info);

/// @brief 获取序列化的人脸feature
/// @param[in]  p_image 图像
/// @param[out] feature 序列化的人脸feature
/// @param[out] feature_len feature长度
/// @param[out] status 函数状态返回
ST_SDK_API
st_result_t st_face_module_get_feature(const st_module_image_t *p_image,
                                       char **feature, int *feature_len,
                                       int *status);

/// @brief 获取序列化的人脸feature和活体分数
/// @param[in]  p_image 图像
/// @param[out] feature 序列化的人脸feature, nullable
/// @param[out] feature_len feature长度, nullable
/// @param[out] image_info 人脸信息返回
/// @param[out] status 函数状态返回
ST_SDK_API
st_result_t st_face_module_get_image_info(const st_module_image_t *p_image,
                                          char **feature, int *feature_len,
                                          st_face_image_info **image_info,
                                          int *status);

/// @brief 通过人脸关键点获取序列化的人脸feature
/// @param[in]  p_image 图像
/// @param[in] points_array 关键点坐标数组
/// @param[in] points_count 关键点个数
/// @param[out] feature 未序列化的人脸feature, nullable
/// @param[out] feature_len feature长度, nullable
ST_SDK_API
st_result_t st_face_module_acquire_feature_by_landmarks(
    const st_image_t *p_image, const st_pointf_t *points_array,
    int points_count, st_feature_t **feature);

/// @brief 比对人脸feature
/// @param[in]  feature1 序列化的人脸feature
/// @param[in]  feature2 序列化的人脸feature
/// @param[out] verify_score 比对分数
ST_SDK_API
st_result_t st_face_module_compare_feature(const char *feature1,
                                           const char *feature2,
                                           float *verify_score);

/// @brief 释放序列化的人脸feature
/// @param[in]  feature 序列化的人脸feature
/// @param[in]  feature_len feature长度
ST_SDK_API
st_result_t st_face_module_release_feature(char *feature, int feature_len);

/// @brief 获取批处理
/// @param[in]  p_image 图像
/// @param[in]  features 序列化的人脸feature
/// @param[in]  feature_count feature个数
/// @param[out] rect 人脸框返回
/// @param[out] verify_score 比对分数返回
/// @param[out] liveness_score 活体分数返回
/// @param[out] status 函数状态返回
ST_SDK_API
st_result_t st_face_module_batch(const st_module_image_t *p_image,
                                 char **features, int feature_count,
                                 st_rect_t *rect, float *verify_score,
                                 float *liveness_score, int *status);

/// @brief
/// 设置活体模型路径，调用此函数后会释放原本的活体模型句柄，用新传入的模型创建句柄
/// @param[in]  model_path 活体模型路径
ST_SDK_API
st_result_t st_face_module_set_liveness_model(const char *model_path);

/// @brief 调用此函数，通过landmarks获取活体分数
/// @param[in]  image 图像数据
/// @param[in]  first_landmark_points 第一张人脸landmarks
/// @param[in] second_landmark_points 第二张人脸landmarks
/// @param[out] score 活体分数
ST_SDK_API
st_result_t st_face_module_acquire_liveness_by_landmarks(
    const st_module_image_t *image, st_pointf_t *first_landmark_points,
    st_pointf_t *second_landmark_points, float *score);

/// @brief 调用此函数获取图像活体分数
/// @param[in]  p_image 图像数据
/// @param[out] liveness_score
/// 活体分数，返回-1时表示图像未检测到人脸，否则返回活体分数0~1
ST_SDK_API
st_result_t st_face_module_acquisition_liveness(
    const st_module_image_t *p_image, int enable_pose, int *pose_checked,
    st_face_image_info **image_info);

/// @brief 调用此函数获取图像最大的人脸框
/// @param[in]  p_image 图像数据
/// @param[in]  orientation 人脸方向
/// @param[out] face_rect 最大的人脸框
/// @param[out] face_count 人脸个数，0表示没有人脸，1表示有人脸
ST_SDK_API
st_result_t st_face_module_get_face_rect(const st_image_t *p_image,
                                         st_face_orientation orientation,
                                         st_rect_t *face_rect, int *face_count);

typedef struct {
  st_rect_t rect;                                         ///< 人脸框
  float detect_score;                                     ///< 人脸检测分数
  st_pointf_t points_array[ST_FACE_LANDMARK_POINTS_CNT];  ///< 人脸关键点
  unsigned char occlusion[ST_FACE_LANDMARK_POINTS_CNT];   ///< 人脸关键点遮挡
  float align_score;                                      ///< align分数
  char *feature_str;                                      ///< 序列化之后的特征
  float liveness_score;                                   ///< 活体分数
  float left_open_score;                                  ///< 左眼开启分数
  float left_valid_score;                                 ///< 左眼开启置信度
  float right_open_score;                                 ///< 右眼开启分数
  float right_valid_score;                                ///< 右眼开启置信度
} st_factory_model_test;

/// @brief 主要用于工厂测试过程中测试模型是否工作正常
/// @param[in]  p_image 图像数据
/// @param[in]  factory_model_test 用于比对的人脸相关数据
/// @param[out] status 函数状态范围，正常为0，否则为对应错误码
ST_SDK_API
st_result_t st_face_factory_model_test(
    const st_module_image_t *p_image,
    const st_factory_model_test *factory_model_test, int *status);

typedef struct {
  int face_count;
  float brightness;
  float verify_score;
  float liveness_score;
  float iou;
} st_factory_test;

/// @brief 工厂测试接口
/// @param[in]  p_image 图像数据
/// @param[in]  feature 人脸feature
/// @param[out] face_count 人脸个数
/// @param[out] brightness 脸部亮度，推荐范围-0.5~0.4
/// @param[out] verify_score 比对分数
/// @param[out] iou 双摄图像脸部重合区域比例
/// @param[out] status 函数状态范围，正常为0，否则为对应错误码
ST_SDK_API
st_result_t st_face_factory_test(const st_module_image_t *p_image,
                                 const char *feature,
                                 st_factory_test *factory_test, int *status);
/// @}

/// @brief
/// 重置人脸认证资源，相当用原配置参数重新创建句柄，调用此函数后，正在进行中的人脸录入过程将会被取消。
ST_SDK_API
st_result_t st_face_module_reset();

/// @brief 销毁人脸认证资源
ST_SDK_API
st_result_t st_face_module_deinit();

/// @defgroup get model input & get model output in batcher
/// @brief for data batcher
///
/// @{
ST_SDK_API
st_result_t st_face_module_prepare_detect_batch_enable(bool enable);

ST_SDK_API
st_result_t st_face_module_prepare_liveness_batch_enable(bool enable);

ST_SDK_API
st_result_t st_face_module_prepare_verify_batch_enable(bool enable);

// get input data
ST_SDK_API
st_result_t st_face_module_detect_get_input_from_pre(const st_image_t *p_image,
                                                     float **input,
                                                     int *input_size);

ST_SDK_API
st_result_t st_face_module_liveness_get_input_from_pre(
    const st_image_t *first_image, st_landmarks_t *first_landmark,
    const st_image_t *second_image, st_landmarks_t *second_landmark,
    float **input1, int *input_size1, float **input2, int *input_size2);

ST_SDK_API
st_result_t st_face_module_verify_get_input_from_pre(const st_image_t *image,
                                                     const st_pointf_t *points,
                                                     int point_count,
                                                     float **input,
                                                     int *input_size);

// run model and get output data
ST_SDK_API
st_result_t st_face_module_detect_get_output_from_input(const float *input,
                                                        const int input_size,
                                                        st_rect_t *face_rect,
                                                        int *face_count);

ST_SDK_API
st_result_t st_face_module_liveness_get_output_from_input(
    const float *input1, const int input_size1, const float *input2,
    const int input_size2, float *liveness_score);

ST_SDK_API
st_result_t st_face_module_verify_get_output_from_input(const float *input,
                                                        const int input_size,
                                                        st_feature_t **feature);
/// @}

#endif  // ST_FACE_MODULE_H_