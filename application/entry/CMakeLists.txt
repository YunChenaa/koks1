# add_executable(koks main.cpp)
if(BUILD_IMAGE_PICK)
  add_executable(koks main_palm_pick.cpp)
elseif(${KOKS_PROJECT} STREQUAL "m53d")
  add_executable(koks main.cpp configure_net.cpp)
else()
  add_executable(koks main.cpp)
  # add_executable(snapyuv kernal_snap_yuv_main.cpp)
endif()

set(KOKS_INCLUDES
  ./
  ${ROOT_PATH}/media_core/include
  ${ROOT_PATH}/communication/include
  ${ROOT_PATH}/common/include
  ${ROOT_PATH}/LCD/include
  ${ROOT_PATH}/ucamera/include
  ${ROOT_PATH}/factory/include
  ${ROOT_PATH}/db/include
)

if(${KOKS_PROJECT} STREQUAL "m53d")
  list(APPEND KOKS_INCLUDES
    ${ROOT_PATH}/wifi/include
    ${ROOT_PATH}/cmcc/include
    ${OVD_PATH}/include)
endif()

target_include_directories(koks PRIVATE ${KOKS_INCLUDES})

target_link_libraries(
  koks
  PRIVATE pthread
          communication
          ${LIB_PLATFORM}
          koks_common
          rt
          dl
          media_core
          app_flash_otp
          ${LIB_FACESDK}
          ${LIB_MAGIK}
          koks_db)

if(${KOKS_PROJECT} STREQUAL "m53d")
  if(BUILD_AIC_WIFI)
    target_link_libraries(
      koks
      PRIVATE atomic koks_wifi
    )
  endif()
  if(BUILD_CMCC_VOIP)
    target_link_libraries(
      koks
      PRIVATE atomic koks_cmcc ${LIB_AAC} ${LIB_CMCC_VOIP}
    )
  endif()
  if(BUILD_CMCC_OVD)
    target_link_libraries(
      koks
      PRIVATE atomic koks_cmcc ${LIB_AAC} ${LIB_OVDSDK}
    )
  endif()
endif()

target_link_options(koks PRIVATE
    -Wl,--no-warn-mismatch
    -Wl,--accept-unknown-input-arch
)

if(BUILD_WITH_SDK_V2)
  if(${KOKS_PROJECT} MATCHES "^(m53a|m24cp|m25p)$")
    target_link_libraries(
      koks
      PRIVATE libgomp.a ${LIB_JPEG}
    )
  else()
    target_link_libraries(
      koks
      PRIVATE libgomp.a
    )
  endif()
else()
  target_link_libraries(
    koks
    PRIVATE ${LIB_JPEG}
  )
endif()


if(${KOKS_PROJECT} STREQUAL "m53b")
  target_link_libraries(
    koks
    PRIVATE LCD ${LIB_OPEN264}
  )
endif()


# add_executable(serial_sample serial_sample.cpp)
# target_include_directories(serial_sample PRIVATE)
# target_link_libraries(serial_sample PRIVATE pthread)

# add_executable(sdk_test sdk_test.cpp)
# target_include_directories(sdk_test PRIVATE ${ROOT_PATH}/communication/include
#                                             ${ROOT_PATH}/ucamera/include
#                                             ${ROOT_PATH}/common/include)
# target_link_libraries(sdk_test PRIVATE pthread communication koks_common
#                                        ${LIB_FACESDK} koks_db app_flash_otp)
