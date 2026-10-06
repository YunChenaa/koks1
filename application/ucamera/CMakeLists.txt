# ucamera统一模块 - 包含uvc和uac子模块
PROJECT(ucamera)

# 获取模块名作为库名
GET_FILENAME_COMPONENT(MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

ADD_DEFINITIONS(-DDUAL_VIDEO)
ADD_DEFINITIONS(-DMODULE_UAC)

# 查找所有源文件
FILE(GLOB SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"             # 主目录源文件
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xu_control/*.c"  # xu_control子目录源文件
)

# 创建统一的静态库
ADD_LIBRARY(${MODULE_NAME} STATIC ${SOURCES})

# 包含所有必要的头文件目录
TARGET_INCLUDE_DIRECTORIES(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src               # 主目录
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xu_control  # xu_control子目录
    ${CMAKE_INSTALL_PREFIX}/include
)

# 链接所有必要的库
# TARGET_LINK_LIBRARIES(${MODULE_NAME} ${COMMON_LIBS})
if(KOKS_PROJECT MATCHES "^(m21a|m21b|m25i|m55)$")
    TARGET_LINK_LIBRARIES(${MODULE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/libs/libusbcamera_1.0.a)
else()
    TARGET_LINK_LIBRARIES(${MODULE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/libs/libusbcamera.a)
endif()

# 如果media库存在，链接media库
# IF(TARGET media)
#     TARGET_LINK_LIBRARIES(${MODULE_NAME} media)
# ENDIF()

# 设置编译定义
TARGET_COMPILE_DEFINITIONS(${MODULE_NAME} PRIVATE 
    MODULE_TAG="${MODULE_NAME}"  # 主模块定义
)