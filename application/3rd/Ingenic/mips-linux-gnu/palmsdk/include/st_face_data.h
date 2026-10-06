#ifndef ST_FACE_DATA_H_
#define ST_FACE_DATA_H_

#include "st_common.h"
#include "st_face_module.h"

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

/// @brief 删除所有已录入的用户
ST_SDK_API
st_result_t st_face_module_delete_all_user();

#endif // ST_FACE_DATA_H_