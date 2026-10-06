#ifndef _ST_UTILS_HH_
#define _ST_UTILS_HH_
#ifdef _MSC_VER
#ifdef ST_STATIC_LIB
#define ST_SDK_API_
#elif defined SDK_EXPORTS
#define ST_SDK_API_ __declspec(dllexport)
#else
#define ST_SDK_API_ __declspec(dllimport)
#endif
#else /* _MSC_VER */
#ifdef SDK_EXPORTS
#define ST_SDK_API_ __attribute__((visibility("default")))
#else
#define ST_SDK_API_
#endif
#endif

#ifdef __cplusplus
#define ST_SDK_API extern "C" ST_SDK_API_
#else
#define ST_SDK_API ST_SDK_API_
#endif

// SDK ERROR CODE
#define ST_OK 0             ///< 正常运行
#define ST_E_INVALIDARG -1  ///< 无效参数,请检查各输入参数的值是否正确
#define ST_E_HANDLE -2  ///< 句柄错误,请检查输入句柄是否已经被初始化
#define ST_E_OUTOFMEMORY -3  ///< 内存不足
#define ST_E_FAIL -4  ///< 运行失败，内部错误，请联系sensetime解决
#define ST_E_INVALID_PIXEL_FORMAT \
  -6  ///< 不支持的图像格式，详细请参考接口说明确认接口支持的图像格式
#define ST_E_FILE_NOT_FOUND -7  ///< 文件不存在，请检查传入路径是否正确
#define ST_E_INVALID_FILE_FORMAT -8  ///< 模型格式不正确导致加载失败
#define ST_E_FILE_EXPIRE -9  ///< 模型文件过期，请检查系统时间是否在模型有效期内
#define ST_E_INVALID_AUTH \
  -13  ///< license不合法（license不正确或license未加载）
#define ST_E_INVALID_APPID \
  -14  ///< 包名错误，请检查packagename(android)和bundle
       ///< id(ios)是否为sdk绑定值一致
#define ST_E_AUTH_EXPIRE -15  ///< SDK过期，请检查系统时间是否在sdk有效期内
#define ST_E_ONLINE_AUTH_CONNECT_FAIL \
  -17  ///< 在线验证连接失败，请检查设备是否可以正确连接互联网
#define ST_E_ONLINE_AUTH_TIMEOUT \
  -18  ///< 在线验证超时，请检查设备是否可以正确连接互联网
#define ST_E_ONLINE_AUTH_INVALID -19       ///< 在线校验失败
#define ST_E_PRODUCT_VERSION_MISMATCH -23  ///< 产品代码版本不一致
#define ST_E_PLATFORM_NOTSUPPORTED -24     ///< 不支持的平台
#define ST_E_UNZIP_FAILED -25              ///< 模型解压缩失败
#define ST_E_SUBMODULE_NON_EXIT -26        ///< 子模型不存在
#define ST_E_HW_AUTH_FAIL -27              ///< license硬件加密验证失败
#endif
