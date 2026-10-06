cmake_minimum_required(VERSION 3.0)
option(BUILD_WITH_SDK_V2 "build with sdk v2" OFF)
option(BUILD_PALM "Build palm project" OFF)
option(BUILD_IMAGE_PICK "Build image pick module" OFF)
option(BUILD_AIC_WIFI "Build aic wifi module" OFF)
option(BUILD_ZBAR "Build zbar module" OFF)
option(BUILD_AAC "Build with aac audio" OFF)
option(BUILD_CMCC_VOIP "Build with cmcc voip module" OFF)
option(BUILD_CMCC_OVD "Build with cmcc ovd module" OFF)
option(BUILD_WSSERVER "Build wsServer module" OFF)
option(BUILD_OVERSEAS "Build overseas version" OFF)
# include(cmake/version.cmake)
include(cmake/toolchain.cmake)
# extract_version() project(sense_koks VERSION ${SENSE_KOKS_VERSION})
# message("version ${SENSE_KOKS_VERSION}")
project(sense_koks)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/install)
macro(SET_IF_NO_DEF VAR_NAME VAR_VALUE)
  if(DEFINED ${VAR_NAME} AND NOT ${VAR_NAME} STREQUAL "")
    # 已经完成定义的就不做处理了
  elseif(DEFINED ENV{VAR_NAME} AND NOT "$ENV{VAR_NAME}" STREQUAL "")
    set(${VAR_NAME} "$ENV{VAR_NAME}")
  else()
    set(${VAR_NAME} "${VAR_VALUE}")
  endif()
endmacro()

set_if_no_def(CMAKE_BUILD_TYPE Debug)

file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/include)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/koks1_config_${KOKS_PROJECT}.h        ${CMAKE_INSTALL_PREFIX}/include/koks1_config.h COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/koks_project_def_${KOKS_PROJECT}.h    ${CMAKE_INSTALL_PREFIX}/include/koks_project_def.h COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/common/include/sys_version_${KOKS_PROJECT}.h ${CMAKE_INSTALL_PREFIX}/include/sys_version.h COPYONLY)

set(3RD_PARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd/Ingenic/mips-linux-gnu)
# set(3RD_PARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd/aarch64)
set(3RD_KOKS_DB_DIR ${3RD_PARTY_PATH}/sqlite)
set(3RD_PLATFORM_DIR ${3RD_PARTY_PATH}/platform)
set(3RD_STB_IMAGE_DIR ${3RD_PARTY_PATH}/stb_image)
set(3RD_FACESDK_DIR ${3RD_PARTY_PATH}/facesdk)
set(3RD_PAMLSDK_DIR ${3RD_PARTY_PATH}/palmsdk)
set(3RD_LIBJPEG_DIR ${3RD_PARTY_PATH}/libturbojpeg)
set(3RD_LIBOPEN264_DIR ${3RD_PARTY_PATH}/open264)


include_directories(${CMAKE_INSTALL_PREFIX}/include)
include_directories(${3RD_KOKS_DB_DIR}/include)
include_directories(${3RD_PLATFORM_DIR}/include)
include_directories(${3RD_STB_IMAGE_DIR}/include)
include_directories(${3RD_LIBJPEG_DIR}/include)
include_directories(${3RD_LIBOPEN264_DIR}/include)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/config)
if(BUILD_WITH_SDK_V2)
include_directories(${3RD_PAMLSDK_DIR}/include)
link_directories(${3RD_PAMLSDK_DIR}/lib)
else()
include_directories(${3RD_FACESDK_DIR}/include)
link_directories(${3RD_FACESDK_DIR}/lib)
endif()

link_directories(${3RD_KOKS_DB_DIR}/lib)
link_directories(${3RD_PLATFORM_DIR}/lib)

link_directories(${3RD_LIBJPEG_DIR}/lib)
link_directories(${3RD_LIBOPEN264_DIR}/lib)

set(SUB_MODULES media_core communication common entry db factory otp ucamera rota LCD)

if(KOKS_PROJECT STREQUAL "m53d")
  set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG")

  add_compile_options(-ffunction-sections -fdata-sections)
  add_link_options(-Wl,--gc-sections)
endif()

file(GLOB LIB_KOKS_DB ${3RD_KOKS_DB_DIR}/lib/*.so)
# file(GLOB LIB_PLATFORM ${3RD_PLATFORM_DIR}/lib/*.so ${3RD_PLATFORM_DIR}/lib/*.a)
set(LIB_PLATFORM
    ${3RD_PLATFORM_DIR}/lib/libimp.a
    ${3RD_PLATFORM_DIR}/lib/libsysutils.a
    ${3RD_PLATFORM_DIR}/lib/libalog.a
    ${3RD_PLATFORM_DIR}/lib/libdl.so.0
)

if(NOT KOKS_PROJECT MATCHES "^(m31|m32|m31a|m33)$")
    list(APPEND LIB_PLATFORM ${3RD_PLATFORM_DIR}/lib/libaudioProcess.so)
endif()

set(LIB_MAGIK
    -Wl,--whole-archive
    ${3RD_PLATFORM_DIR}/lib/liburanus_tensor.a
    ${3RD_PLATFORM_DIR}/lib/liburanus_core.a
    ${3RD_PLATFORM_DIR}/lib/liburanus_extend.a
    ${3RD_PLATFORM_DIR}/lib/libaie.a
    -Wl,--no-whole-archive
)

if(BUILD_PALM AND BUILD_IMAGE_PICK)
  message("Build palm image pick version")
  add_definitions(-DPALM_IMAGE_PICK)
endif()

if(BUILD_WSSERVER)
  message("Build with wsServer")
  set(LIB_WSSERVER ${3RD_PARTY_PATH}/wsServer/lib/libws.a)
  include_directories(${3RD_PARTY_PATH}/wsServer/include)
endif()

if(BUILD_ZBAR)
  message("Build with zbar")
  set(LIB_ZBAR ${3RD_PARTY_PATH}/zbar/lib/libzbar.a)
  include_directories(${3RD_PARTY_PATH}/zbar/include)
endif()

if(BUILD_AAC)
  message("Build with aac audio")
  add_definitions(-DHAVE_FDK_AAC)
  set(LIB_AAC ${3RD_PARTY_PATH}/fdk-aac/lib/libfdk-aac.a)
  include_directories(${3RD_PARTY_PATH}/fdk-aac/include)
endif()

if(BUILD_AIC_WIFI)
  message("Build with aic wifi module")
  add_definitions(-DKOKS_FEATURE_WIFI)
  list(APPEND SUB_MODULES wifi)
endif()

if(BUILD_CMCC_OVD)
  message("Build with cmcc ovd module")
  add_definitions(-DKOKS_FEATURE_OVD)
  set(OVD_PATH ${3RD_PARTY_PATH}/cmcc_ovd)
  list(APPEND SUB_MODULES cmcc)
  set(LIB_SSL
      ${OVD_PATH}/lib/libmbedtls.a
      ${OVD_PATH}/lib/libmbedx509.a
      ${OVD_PATH}/lib/libmbedcrypto.a
  )
  set(LIB_LIVE555_PATH ${3RD_PARTY_PATH}/live555)
  set(LIB_LIVE555
      ${LIB_LIVE555_PATH}/lib/libBasicUsageEnvironment.a
      ${LIB_LIVE555_PATH}/lib/libgroupsock.a
      ${LIB_LIVE555_PATH}/lib/libliveMedia.a
      ${LIB_LIVE555_PATH}/lib/libUsageEnvironment.a
  )

  set(LIB_OVDSDK
      ${OVD_PATH}/lib/libcjson.a
      ${LIB_SSL}
      ${LIB_LIVE555}
      ${OVD_PATH}/lib/libovdsdk.a
  )
  
endif()

if(BUILD_CMCC_VOIP)
  message("Build with cmcc voip module")
  add_definitions(-DCMCC_VOIP)
  set(LIB_CMCC_VOIP ${3RD_PARTY_PATH}/cmcc_ovd/lib/libvoipengine_not_include_mbedtls.a)
endif()

if(BUILD_WITH_SDK_V2)
  if(KOKS_PROJECT MATCHES "^(m53a|m24cp)$")
    message("Build with sdk v2 for m53a or m24cp")
    set(LIB_FACESDK
        ${3RD_PAMLSDK_DIR}/lib/libalgo_sdk_core.a
        ${3RD_PAMLSDK_DIR}/lib/libalgo_sdk_m5-3a_m2-4cp.a
    )
  elseif(KOKS_PROJECT MATCHES "^(m25p)$")
    message("Build with sdk v2 for m25p")
    set(LIB_FACESDK
        ${3RD_PAMLSDK_DIR}/lib/libalgo_sdk_core.a
        ${3RD_PAMLSDK_DIR}/lib/libalgo_sdk_m2-5p.a
    )
  else()
    set(LIB_FACESDK
        ${3RD_PAMLSDK_DIR}/lib/libalgo_sdk_core.a
        ${3RD_PAMLSDK_DIR}/lib/libalgo_sdk.a
    )
  endif()
else()
  if(KOKS_PROJECT STREQUAL "m24b3")
    set(LIB_FACESDK
        ${3RD_FACESDK_DIR}/lib/libalgo_sdk_core.a
        ${3RD_FACESDK_DIR}/lib/libalgo_sdk_single.a
    )
  elseif(KOKS_PROJECT STREQUAL "m25i")
    set(LIB_FACESDK
        ${3RD_FACESDK_DIR}/lib/libalgo_sdk_core.a
        ${3RD_FACESDK_DIR}/lib/libalgo_sdk_oversea.a
    )
  else()
    set(LIB_FACESDK
        ${3RD_FACESDK_DIR}/lib/libalgo_sdk_core.a
        ${3RD_FACESDK_DIR}/lib/libalgo_sdk.a
    )
  endif()
endif()

file(GLOB LIB_JPEG ${3RD_LIBJPEG_DIR}/lib/*.a)
file(GLOB LIB_OPEN264 ${3RD_LIBOPEN264_DIR}/lib/*.a)

set(ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

foreach(module ${SUB_MODULES})
  add_subdirectory(${module})
endforeach()

foreach(module ${SUB_MODULES})
  install(PROGRAMS ${CMAKE_SOURCE_DIR}/build/entry/koks
                   ${CMAKE_SOURCE_DIR}/build/rota/rota
          DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
  install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/build/${module}/ ${3RD_PLATFORM_DIR}/lib/
              ${3RD_LIBJPEG_DIR}/lib/ ${3RD_LIBOPEN264_DIR}/lib/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/libs/
    FILES_MATCHING
    PATTERN "*.so*"
    PATTERN "*CMakeFiles*" EXCLUDE)
endforeach()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/config
        DESTINATION ${CMAKE_INSTALL_PREFIX}/
        PATTERN "*.h" EXCLUDE)

install(PROGRAMS ${CMAKE_SOURCE_DIR}/script/run.sh
                 ${CMAKE_SOURCE_DIR}/script/koks_daemon.sh
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

if(BUILD_WITH_SDK_V2)
  install(DIRECTORY ${3RD_PAMLSDK_DIR}/db/face DESTINATION ${CMAKE_INSTALL_PREFIX}/config/db)
  install(FILES ${CMAKE_SOURCE_DIR}/palm-models/alignment_t33_magik.mgk
                ${CMAKE_SOURCE_DIR}/palm-models/feature_t33_magik.mgk
                ${CMAKE_SOURCE_DIR}/palm-models/detection_t33_magik.mgk
                ${CMAKE_SOURCE_DIR}/palm-models/model_index.txt
          DESTINATION ${CMAKE_INSTALL_PREFIX}/models)
  if(KOKS_PROJECT MATCHES "^(m31|m32|m31a|m33)$")
    install(FILES ${CMAKE_SOURCE_DIR}/palm-models/live2i_t33_magik.mgk
            DESTINATION ${CMAKE_INSTALL_PREFIX}/models)
  elseif(BUILD_OVERSEAS STREQUAL "ON")
    install(FILES ${CMAKE_SOURCE_DIR}/face-models/live2i_t33_magik_oversea.mgk
            DESTINATION ${CMAKE_INSTALL_PREFIX}/models
            RENAME live2i_t33_magik.mgk)
  else()
    install(FILES ${CMAKE_SOURCE_DIR}/face-models/live2i_t33_magik.mgk
            DESTINATION ${CMAKE_INSTALL_PREFIX}/models)
  endif()
  if(BUILD_PALM)
    install(DIRECTORY ${3RD_PAMLSDK_DIR}/db/palm DESTINATION ${CMAKE_INSTALL_PREFIX}/config/db)
    install(FILES ${CMAKE_SOURCE_DIR}/palm-models/palm_alignment_t33_magik.mgk
                  ${CMAKE_SOURCE_DIR}/palm-models/palm_feature_t33_magik.mgk
                  ${CMAKE_SOURCE_DIR}/palm-models/palm_live2i_t33_magik.mgk
            DESTINATION ${CMAKE_INSTALL_PREFIX}/models)
  endif()
else()
  install(DIRECTORY ${3RD_FACESDK_DIR}/db/ DESTINATION ${CMAKE_INSTALL_PREFIX}/config/db)
  install(FILES ${CMAKE_SOURCE_DIR}/face-models/alignment_t33_magik.mgk
                ${CMAKE_SOURCE_DIR}/face-models/detection_t33_magik.mgk
                ${CMAKE_SOURCE_DIR}/face-models/feature_t33_magik.mgk
                ${CMAKE_SOURCE_DIR}/face-models/model_index.txt
          DESTINATION ${CMAKE_INSTALL_PREFIX}/models)
  if(KOKS_PROJECT STREQUAL "m24b3")
    install(FILES ${CMAKE_SOURCE_DIR}/face-models/live2i_t33_magik_mono_ir.mgk
            DESTINATION ${CMAKE_INSTALL_PREFIX}/models
            RENAME live2i_t33_magik.mgk)
  elseif(KOKS_PROJECT STREQUAL "m25i")
    install(FILES ${CMAKE_SOURCE_DIR}/face-models/live2i_t33_magik_oversea.mgk
            DESTINATION ${CMAKE_INSTALL_PREFIX}/models
            RENAME live2i_t33_magik.mgk)
  else()
    install(FILES ${CMAKE_SOURCE_DIR}/face-models/live2i_t33_magik.mgk
            DESTINATION ${CMAKE_INSTALL_PREFIX}/models)
  endif()
endif()

if(KOKS_PROJECT STREQUAL "m53d")
    # install(DIRECTORY ${CMAKE_SOURCE_DIR}/cmccfs/
    #        DESTINATION ${CMAKE_INSTALL_PREFIX}/cmcc_fs)
    
    install(FILES ${CMAKE_SOURCE_DIR}/cmcc/screen.nv12
           DESTINATION ${CMAKE_INSTALL_PREFIX}/cmcc_fs)

    install(FILES ${CMAKE_SOURCE_DIR}/cmcc/di.wav
           DESTINATION ${CMAKE_INSTALL_PREFIX}/cmcc_fs)

    # install(FILES ${CMAKE_SOURCE_DIR}/rootfs/etc/shadow
    #        DESTINATION ${CMAKE_INSTALL_PREFIX}/etc)
    # install(FILES ${CMAKE_SOURCE_DIR}/rootfs/etc/shadow-
    #        DESTINATION ${CMAKE_INSTALL_PREFIX}/etc)
endif()
