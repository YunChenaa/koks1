#ifndef ST_FACE_DATA_H_
#define ST_FACE_DATA_H_

#include "st_common.h"
#include "st_face_module.h"

/// 用户信息
typedef struct st_module_user_t {
  int id;              ///< 用户id
  int admin;           ///< 0表示不是管理员，否则是管理员
  char name[32];       ///< 用户名
  int gender;          ///< 用户性别
  int server_user_id;  ///< 由外部指定，与id参数不同，默认为0
  char avatar[32];     ///< 用户图像参数
  char rgb_enroll;  ///< 由外部指定， 确认是否为RGB图片注册的用户
} st_module_user_t;

/// @brief 根据用户id获取单个用户信息
/// @param[in]  user_id 用户id
/// @param[out] user 返回的用户信息
ST_SDK_API
st_result_t st_face_module_get_user(int user_id, st_module_user_t *user);

/// @brief 获取所有用户信息
/// @param[out] users
/// 已录入的所有用户信息，由sdk分配内存，需要调用st_face_module_delete_users_buffer释放
/// @param[out] count 用户个数
ST_SDK_API
st_result_t st_face_module_get_all_users(st_module_user_t **users, int *count);

/// @brief 删除用户信息buffer
/// @param[in] users 用户信息，由st_face_module_get_all_users获取
/// @param[in] count 用户个数
ST_SDK_API
st_result_t st_face_module_delete_users_buffer(st_module_user_t *users,
                                               int count);

/// @brief 人脸解锁
/// @param[in]  user 用户数据，更新user->id对应用户的信息
ST_SDK_API
st_result_t st_face_module_update_user(const st_module_user_t *user);

/// @brief 删除已录入的用户
/// @param[in]  user_id 已录入的用户id
ST_SDK_API
st_result_t st_face_module_delete_user(int user_id);

/// @brief 删除用户列表
/// @param[in] user_list 已录入的用户id列表
ST_SDK_API
st_result_t st_face_module_delete_users_list(int users_list[], int count);

/// @brief 删除所有已录入的用户
ST_SDK_API
st_result_t st_face_module_delete_all_user();

/// @brief 根据server_user_id获取user_id
/// @param[in]  server_user_id 服务器传的用户id
/// @param[out] user_id sdk里的用户id，返回-1表示没有这个用户
ST_SDK_API
st_result_t st_face_module_get_user_id(int server_user_id, int *user_id);

#endif  // ST_FACE_DATA_H_