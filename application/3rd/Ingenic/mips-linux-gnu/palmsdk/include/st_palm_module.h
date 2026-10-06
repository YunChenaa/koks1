#ifndef ST_PALM_MODULE_H_
#define ST_PALM_MODULE_H_

#include <vector>

#include "st_face_data.h"
#include "st_face_module.h"
constexpr int ST_PALM_LANDMARK_POINTS_CNT = 50;
#define ST_PALM_MODULE_STATUS_NORMAL 1001        ///< 手掌正常
#define ST_PALM_MODULE_STATUS_NO_PALM 1002       ///< 为检测到手掌
#define ST_PALM_MODULE_STATUS_RECT_CONF_LOW 1003 ///< 手掌检测框置信度太低
#define ST_PALM_MODULE_STATUS_LANDMARK_CONF_LOW 1004 ///< 手掌关键点置信度太低
#define ST_PALM_MODULE_STATUS_ANGLE_ROLL_ERROR 1005 ///< 角度不符合要求——侧倾
#define ST_PALM_MODULE_STATUS_ANGLE_LEAN_ERROR 1006 ///< 角度不符合要求——前倾
#define ST_PALM_MODULE_STATUS_BLOCK_1 1007 ///< 手掌遮挡 1 ??? 为啥分 1,2
#define ST_PALM_MODULE_STATUS_BLUR_1 1008  ///< 人手掌模糊 1 ???
#define ST_PALM_MODULE_STATUS_PALM_BACK_OR_GLOVE 1009 ///< 手背或手套
#define ST_PALM_MODULE_STATUS_GESTURE 1010            ///< 异常手势
#define ST_PALM_MODULE_STATUS_OVER_EXPOSED 1011 ///< 过曝光，未通过质量分
#define ST_PALM_MODULE_STATUS_UNDER_EXPOSED 1012 ///< 欠曝光，未通过质量分
#define ST_PALM_MODULE_STATUS_BLOCK_2 1013       ///< 手掌遮挡 2
#define ST_PALM_MODULE_STATUS_BLUR_2 1014        ///< 手掌模糊 2
#define ST_PALM_MODULE_STATUS_WATER 1015         ///< 手掌有水
#define ST_PALM_MODULE_STATUS_TOO_CLOSE 1016     ///< 手掌太近
#define ST_PALM_MODULE_STATUS_TOO_FAR 1017       ///< 手掌太远
#define ST_PALM_MODULE_STATUS_IR_NOT_LIVE 1018   ///< 活体不过
#define ST_PALM_MODULE_STATUS_COMPARE_FAIL 1019  ///< 手掌比对失败
#define ST_PALM_MODULE_STATUS_REFLECT 1020       ///< 手掌反光
#define ST_PALM_MODULE_STATUS_OUT_LEFT 1021      ///< 手掌左侧出框
#define ST_PALM_MODULE_STATUS_OUT_RIGHT 1022     ///< 手掌右侧出框
#define ST_PALM_MODULE_STATUS_OUT_TOP 1023       ///< 手掌顶部出框
#define ST_PALM_MODULE_STATUS_OUT_BOTTOM 1024    ///< 手掌底部出框
#define ST_PALM_MODULE_STATUS_OUT_LT 1025        ///< 手掌左和上出框
#define ST_PALM_MODULE_STATUS_OUT_LB 1026        ///< 手掌左和下出框
#define ST_PALM_MODULE_STATUS_OUT_RT 1027        ///< 手掌右和上出框
#define ST_PALM_MODULE_STATUS_OUT_RB 1028        ///< 手掌右和下出框
#define ST_PALM_MODULE_STATUS_SYS_ERROR 1029     ///< 系统错误
#define ST_PALM_MODULE_STATUS_UNKNOWN_ERROR 1030 ///< 未知错误
// enroll
#define ST_PALM_MODULE_STATUS_MAXIMUM_USER_NUMBER                              \
  1100 ///< 达到了录入的最大用户数
#define ST_PALM_MODULE_STATUS_UNFINISHED                                       \
  1101 ///< 手掌录入未完成，继续调用录入接口, 重点新增
#define ST_PALM_MODULE_STATUS_PALM_ADDED 1102 ///< 录入阶段，手掌已添加
#define ST_PALM_MODULE_STATUS_TIMEOUT 1103    ///< 手掌录入或解锁超时
#define ST_PALM_MODULE_STATUS_ABORT 1104 ///< 手掌录入/解锁算法已终止
#define ST_PALM_MODULE_STATUS_MODEL_MISSMATCH 1105 ///< 手掌算法模型版本不对
#define ST_PALM_MODULE_STATUS_MODEL_TOO_LARGE 1106 ///< 手掌算法模型大小超过阈值
#define ST_PALM_MODULE_STATUS_INTERFACE_EXCEPTION 1107 ///< 手掌算法接口异常
#define ST_PALM_MODULE_STATUS_INTERFACE_ERROR 1108 ///< 手掌算法接口错误
#define ST_PALM_MODULE_STATUS_SWITCH_TO_PALM 1109  ///< 手掌识别切换
#define ST_PALM_MODULE_STATUS_NO_TEMPLATE 1110 ///< 没有已录入的手掌用户
#define ST_PALM_MODULE_STATUS_DUPLICATE_ENROLL 1111 ///< 重复录入
#define ST_PALM_MODULE_STATUS_NO_SUCH_USER 1112     ///< 无此用户
#define ST_PALM_MODULE_STATUS_EXISTING_USER_ID_ERROR                           \
  1113 ///< 录入时，存在相同用户ID
#define ST_PALM_MODULE_STATUS_INVALID_USER_ID_ERROR 1114 ///< 用户ID无效
// unlcok
#define ST_PALM_MODULE_STATUS_UNLOCK_OK 1200     ///< 手掌解锁成功
#define ST_PALM_MODULE_STATUS_UNLOCK_FAILED 1201 ///< 手掌解锁失败
#define ST_PALM_MODULE_STATUS_MORE_HACKNESS_FAILED                             \
  1202 ///< 假体次数比活体次数多了5次

#define ST_PALM_MODULE_STATUS_FAKE_HACKNESS_FAILED                             \
  1203 ///< 活体算法判断为真人， 但是需要当做假体对待

enum PalmStatus {
  // 成功
  kOk = 0, // 手掌正常，操作成功

  // 检测与基本有效性问题
  kError_Liveness_Failed, // 活体未通过
  kError_NoPalmDetected,  // 未检测到手掌
  kError_BackOfHand,      // 检测到的是手背

  // 图像质量问题
  kError_Quality_LowBoxConfidence,      // 手掌框关键点置信度过低
  kError_Quality_LowKeypointConfidence, // 手掌关键点置信度过低

  kError_Quality_Overexposed,  // 过曝光
  kError_Quality_Underexposed, // 欠曝光

  // 注册与认证
  kError_Registration_NotEnough,   // 注册数量不足
  kError_Authentication_DataEmpty, // 认证数据区为空

  // 算法核心
  kError_Algorithm_ComparisonFailed, // 手掌比对失败 (包括匹配分过低)

  // 推理系统
  kError_Timeout, // 超时错误
  kError_Sys,     // API错误
};

enum PalmConfidenceLevel {
  _kVeryLow, // 极低：最宽松，容忍度最高，可能导致误识率(FAR)上升
  _kLow,    // 低
  _kNormal, // 正常：平衡了安全性和便捷性，为推荐的默认值
  _kHigh,   // 高
  _kVeryHigh // 极高：最严格，安全性最高，可能导致拒识率(FRR)上升
};

struct PalmModuleLevel {
  enum PalmConfidenceLevel detection_levle;
  enum PalmConfidenceLevel brightness_levle;
  enum PalmConfidenceLevel match_levle;
  enum PalmConfidenceLevel liveness_levle;
};
/// 手掌信息
typedef struct st_module_palm_info_t {
  int left;    ///< 手掌矩形框最左边的坐标
  int top;     ///< 手掌矩形框最上边的坐标
  int right;   ///< 手掌矩形框最右边的坐标
  int bottom;  ///< 手掌矩形框最下边的坐标
  float yaw;   ///< 围绕Y轴旋转，也叫偏航角
  float pitch; ///< 围绕X轴旋转，也叫做俯仰角
  float roll;  ///< 围绕Z轴旋转，也叫翻滚角
} st_module_palm_info_t;

/// 句柄配置参数
typedef struct st_module_palm_config_t {
  char *template_save_path;     ///< 模板存储路径
  bool liveness_enable;         ///< 是否开启活体检测
  callback_calc_pose calc_pose; ///< 设置手掌姿态回调函数，可以为NULL
  callback_rect palm_rect;      ///< 设置手掌框回调函数，可以为NULL
  callback_algo_abort algo_abort; ///< 算法中断回调，可以为NULL
  struct PalmModuleLevel palm_levle;
} st_module_palm_config_t;

/// 内部参数配置
typedef struct st_module_palm_param_config_t {
  // argorithm threshold
  float config_align_threshold;    ///< 关键点阈值，default 0.xx
  float config_landmark_threshold; ///< 关键点阈值，default 0.xx
  float config_hackness_threshold; ///< 活体阈值，default xx
  float config_verify_threshold;   ///< 比对阈值，default xx
  float config_enroll_verify_threshold;
  float config_quality_brightness_max;   ///< 亮度阈值上限，default 0.5f
  float config_quality_brightness_min;   ///< 亮度阈值下限，default -0.5f
                                         // 手掌距离
  float config_palm_close_threshold;     ///< 最近阈值
                                         ///< (palm_rect.right-palm_rect.left)
  float config_palm_far_threshold;       ///< 最远阈值
                                         ///< (palm_rect.right-palm_rect.left)
  float config_palm_detect_rect_margin;  ///< 手掌检测框边距像素
  float config_palm_align_rect_margin;   ///< 手掌关键点框边距像素
  float config_palm_sharpness_threshold; ///< 手掌清晰度阈值, default:0.1f
  int config_pixel_to_image_edge; ///< 手掌距离图像边缘的像素个数, default:5

  // palm keypoint stabilizer drift thresholds（稳定帧漂移阈值，未配置时默认 [15,30]）
  //每帧的阈值的计算方式如下
  //rect_width 为 sdk palm keypoint square rect: left=%d, top=%d, right=%d, bottom=%d 打印出来的 right和left的差值
  //float ratio =(rect_width - config.config_palm_far_threshold) /(config.config_palm_close_threshold - config.config_palm_far_threshold);
  //每帧最终实时配置的漂移阈值distance_threshold = min_drift + ratio * (max_drift - min_drift);
  int config_palm_min_drift_threshold; ///< 稳定帧最小漂移阈值(手掌远距config_palm_far_threshold/框小), default:15
  int config_palm_max_drift_threshold; ///< 稳定帧最大漂移阈值(手掌近距config_palm_close_threshold/框大), default:30

  // enroll direction parameters
  int config_enroll_yaw_min;   ///< 注册 yaw 阈值
  int config_enroll_yaw_max;   ///< 注册 yaw 阈值
  int config_enroll_roll_min;  ///< 注册 roll 阈值
  int config_enroll_roll_max;  ///< 注册 roll 阈值
  int config_enroll_pitch_min; ///< 注册 pitch 阈值
  int config_enroll_pitch_max; ///< 注册 pitch 阈值

  // palm occlusion parameters
  float config_occlusion_ratio; ///< 手掌遮挡比例阈值

  // verify direction parameters
  int config_verify_yaw_min;   ///< default -20
  int config_verify_yaw_max;   ///< default 20
  int config_verify_roll_min;  ///< default -25
  int config_verify_roll_max;  ///< default 25
  int config_verify_pitch_min; ///< default -35
  int config_verify_pitch_max; ///< default 40
  int enable_dup_enroll_;      //防止重复录入 1-开启 0-关闭
  int enable_blur_;            //是否开启模糊检测 1-开启 0-关闭
} st_module_palm_param_config_t;

typedef struct st_module_palm_algo_info {
  st_rect_t palm_rect;
  int occlusion_points[ST_PALM_LANDMARK_POINTS_CNT];
  float iou_score;
  float quality_score;
  float verify_score;
  int verify_id;
  float liveness_score;
  int status;
} st_module_palm_algo_info;

/// @brief 初始化手掌认证资源
/// @param[in]  config 配置参数
/// 需要先调用st_face_module_init
ST_SDK_API
st_result_t st_palm_module_init(const st_module_palm_config_t *config);

/// @brief 设置内部各项参数
/// @param[in]  config 配置参数
ST_SDK_API
st_result_t
st_palm_module_set_param_config(const st_module_palm_param_config_t *config);

/// @brief 人脸手掌二合一检测，只返回最大的人脸和手掌，每个目标最多一个框
/// @param[in]  p_image 图像数据
/// @param[out] face_rect 人脸检测框
/// @param[out]  palm_rect 手掌检测框
/// @param[out] opisthenar_rect 手背检测框
/// @param[out] status 只检测到手背返回ST_PALM_MODULE_STATUS_PALM_BACK
/// @param[in] mode 检测模式,参考 st_module_detect_mode
/// 检测和质量拆分为两个接口的考量：IR 为主,需要做质量判断，Y/RGB
/// 可能不需要质量判断，但是又希望把检测从人脸和手掌的流程中都独立出来，识别或者注册流程更纯粹，完全不做任何检测
/// AE
ST_SDK_API
st_result_t st_face_plam_detect(const st_image_t *p_image, st_rect_t *face_rect,
                                st_rect_t *palm_rect,
                                st_rect_t *opisthenar_rect, int *status,
                                st_module_detect_mode mode);

/// @brief 人脸质量判断
/// @param[in]  p_image 图像数据
/// @param[in]  face_rect 人脸检测框
/// @param[in]  is_enroll 是否是注册，0 - 解锁， 1 - 注册，注册要求比解锁更严苛
/// @param[out] status 人脸状态，当状态为 ST_FACE_MODULE_STATUS_NORMAL
/// @param[out] brightness 基于rect_out计算的人脸区域亮度
/// @param[out] rect_out 人脸小框
/// 人脸区域的亮度值，范围[-1,1]，0为最佳亮度，负数表示偏暗，正数表示偏亮
/// 手掌状态，当状态为 ST_FACE_MODULE_STATUS_NORMAL
/// 时可以走后续流程，其他时候反馈比如太近，太远，太靠边缘等
ST_SDK_API st_result_t st_face_quality(const st_image_t *p_image,
                                       st_rect_t face_rect, int is_enroll,
                                       int *status, double *_brightness,
                                       st_rect_t *rect_out);

/// @brief 手掌质量判断
/// @param[in]  p_image 手掌图像数据
/// @param[in]  palm_rect 人脸检测框
/// @param[in]  is_enroll 是否是注册，0 - 解锁， 1 - 注册，注册要求比解锁更严苛
/// @param[out] status 手掌状态，当状态为 ST_PALM_MODULE_STATUS_NORMAL
/// @param[out] brightness
/// 手掌区域的亮度值，范围[-1,1]，0为最佳亮度，负数表示偏暗，正数表示偏亮
/// 才走后续的手掌流程，当手掌为其他状态是，需要的时候把状态上报给上位机，比如角度大，太近，太远等

ST_SDK_API st_result_t st_palm_quality(const st_image_t *p_image,
                                       st_rect_t palm_rect, int is_enroll,
                                       int *status, double *_brightness);

/// @brief 设置录入模式
/// @param[in]  mode 录入模式
/// ST_MODULE_ENROLL_MODE_INTERFACE/ST_MODULE_ENROLL_MODE_NORMAL
ST_SDK_API SDK_DEPRECATED("接口即将移除") st_result_t
    st_palm_module_set_enroll_mode(st_module_enroll_mode mode);

/// @brief 手掌录入
/// @param[in]  p_first_image 手掌图像数据
/// @param[in]  first_rect 手掌检测框，内部不需要再做手掌检测
/// @param[in]  p_second_image 手掌图像数据
/// @param[in]  second_rect 手掌检测框，内部不需要再做手掌检测
/// @param[out] rect_out 手掌对齐后的检测框
/// @param[out] status 手掌录入状态返回
/// @param[in] mode 录入的用户id
/// 如果指定ID模式下，返回指定的ID（ID是小于65530的正整数），否则返回内部生成的ID
/// @param[out] user_id
/// 录入的用户id，当status为 ST_PALM_MODULE_STATUS_PALM_ADDED 时有效，当
/// status为 ST_PALM_MODULE_STATUS_UNFINISHED，
/// 时，需要继续调用此接口直至成功或者失败（-1）
/// @param[out] orientation 用来区分左手还是右手 0 左手 1 右手
/// @param[out] brightness
/// 手掌区域的亮度值，范围[-1,1]，0为最佳亮度，负数表示偏暗，正数表示偏亮
/// @param[in]  user_info 用户信息，录入时传入，需要传入admin name gender
/// rgb_enroll四个信息
/// @param[in] first_enroll 是否是第一次录入，true - 第一次录入，false -
/// 超时时间内后续录入
ST_SDK_API
st_result_t
st_palm_module_enroll(const st_image_t *p_first_image, st_rect_t first_rect,
                      const st_image_t *p_second_image, st_rect_t second_rect,
                      st_rect_t *rect_out, int *status,
                      st_module_enroll_id_mode mode, int *user_id,
                      int *orientation, double *brightness,
                      st_module_user_t user_info, bool first_enroll);

ST_SDK_API
st_result_t st_palm_module_get_align_rect(const st_image_t *p_first_image,
                                          st_rect_t first_rect,
                                          st_rect_t *rect_out, int *cross);

/// @brief 手掌解锁
/// @param[in] p_first_image 手掌图像数据
/// @param[in] first_rect 手掌检测框，内部不需要再做手掌检测
/// @param[in] p_second_image 手掌图像数据
/// @param[in] second_rect 手掌检测框，内部不需要再做手掌检测
/// @param[out] rect_out 手掌关键点小框
/// @param[out] status 手掌解锁状态返回
/// @param[out] user_id 解锁成功的用户id
/// @param[out] brightness
/// 手掌区域的亮度值，范围[-1,1]，0为最佳亮度，负数表示偏暗，正数表示偏亮
/// @param[in] first_enroll 是否是第一次解锁，true - 第一次录入，false -
/// 超时时间内后续录入
ST_SDK_API
st_result_t st_palm_module_unlock(const st_image_t *p_first_image,
                                  st_rect_t first_rect,
                                  const st_image_t *p_second_image,
                                  st_rect_t second_rect, st_rect_t *rect_out,
                                  int *status, int *user_id, double *brightness,
                                  bool first_unlock);

/// @brief 手掌特征提取
///
/// 此函数用于从单张图像中提取手掌特征数据。传入图像及对应的手掌框，
/// 内部执行关键点对齐后提取特征，通过st_feature_t返回特征数据。
///
/// @param[in] image 手掌图像数据。
/// @param[in] palm_rect 手掌检测框。
/// @param[out] feature 返回的手掌特征数据，需通过st_face_module_delete_data释放。
/// @return 返回函数执行结果，ST_OK表示成功，ST_PALM_MODULE_STATUS_LANDMARK_CONF_LOW表示对齐失败，
/// ST_PALM_MODULE_STATUS_COMPARE_FAIL表示特征提取失败。
ST_SDK_API
st_result_t st_palm_module_extract_feature(const st_image_t *image,
                                           st_rect_t palm_rect,
                                           st_feature_t **feature);

/// @brief 设置是否进行手掌比对，0为关闭手掌比对，否则为打开
ST_SDK_API
st_result_t st_palm_module_set_enable_verify(int enable);

/// @brief 设置是否进行质量检测，0为关闭质量检测，否则为打开，default 1
ST_SDK_API SDK_DEPRECATED("接口即将移除") st_result_t
    st_palm_module_set_enable_quality(int enable);

/// @brief 获取已注册掌静脉用户的所有 ID
/// @param[out] user_id_list id 列表
/// @param[out] user_count 返回的用户个数
ST_SDK_API SDK_DEPRECATED("接口即将移除") st_result_t
    st_palm_module_all_user_ids(int *user_id_list, int *user_count);

/// @brief 设置最大录入的数量，如果不调用，则默认50人
ST_SDK_API st_result_t st_palm_module_set_max_user_count(int count);

/// @brief 获取sdk版本号
/// @return 返回版本号字符串
ST_SDK_API SDK_DEPRECATED("接口即将移除") st_result_t
    st_palm_module_get_version(unsigned char *version);

/// @brief 销毁手掌认证资源
ST_SDK_API st_result_t st_palm_module_deinit();

/// @brief 根据用户id获取单个用户信息
/// @param[in]  user_id 用户id
/// @param[out] user 返回的用户信息
ST_SDK_API
st_result_t st_palm_module_get_user(int user_id, st_module_user_t *user);

/// @brief 获取所有用户信息
/// @param[out] users
/// 已录入的所有用户信息，由sdk分配内存，需要调用st_palm_module_delete_users_buffer释放
/// @param[out] count 用户个数
ST_SDK_API
st_result_t st_palm_module_get_all_users(st_module_user_t **users, int *count);

/// @brief 删除用户信息buffer
/// @param[in] users 用户信息，由st_palm_module_get_all_users获取
/// @param[in] count 用户个数
ST_SDK_API
st_result_t st_palm_module_delete_users_buffer(st_module_user_t *users,
                                               int count);

/// @brief 人脸解锁
/// @param[in]  user 用户数据，更新user->id对应用户的信息
ST_SDK_API SDK_DEPRECATED("接口即将移除") st_result_t
    st_palm_module_update_user(const st_module_user_t *user);

/// @brief 删除已录入的用户
/// @param[in]  user_id 已录入的用户id
ST_SDK_API
st_result_t st_palm_module_delete_user(int user_id);

/// @brief 删除所有已录入的用户
ST_SDK_API
st_result_t st_palm_module_delete_all_user();

/// @brief 设置是否允许用户重复录入, 0为不允许重复录入，否则为允许，default 1
ST_SDK_API
st_result_t st_palm_module_set_enable_dup_enroll(int enable);

/// @brief 设置是否开启手掌模糊检测，0为关闭手掌模糊检测，否则为打开，default 1
ST_SDK_API SDK_DEPRECATED("接口即将移除") st_result_t
    st_palm_module_set_enable_blur(int enable);

/**
 * @name: st_palm_module_get_models_md5
 * @brief: 获取多个模型的MD5值
 * @param {int} *model_count 模型数量
 * @param {char***} md5_outputs
 * 输出的MD5字符串数组，由SDK分配内存，调用者需要调用st_palm_module_delete_md5_buffer释放内存
 * @return {*}
 */
ST_SDK_API
st_result_t st_palm_module_get_models_md5(int *model_count,
                                          char ***md5_outputs);

/*
 * @name: st_palm_sfc_init
 * @brief: SFC初始化接口，提供给外部调用以初始化SFC相关资源
 * @param {uint32_t} model_offset 模型偏移地址，单位为字节
 * @param {const char*} sfc_device SFC设备路径，例如"/dev/sfc"
 * @return {*}
 */
ST_SDK_API st_result_t st_palm_sfc_init(uint32_t model_offset,
                                        const char *sfc_device);
#endif // ST_PALM_MODULE_H_