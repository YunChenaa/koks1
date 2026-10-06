if(BUILD_AAC)
add_library( media_core STATIC ./src/st_koks_mediamanager.cpp
                               ./src/st_koks_isp.cpp
                               ./src/st_koks_uvc.cpp
                               ./src/st_koks_uvc_factory.cpp
                               ./src/st_koks_video.cpp
                               ./src/st_koks_audio.cpp
                               ./src/aac_encoder.cpp
                               ./src/jpeg_encoder.cpp)
else()
add_library( media_core STATIC ./src/st_koks_mediamanager.cpp
                               ./src/st_koks_isp.cpp
                               ./src/st_koks_uvc.cpp
                               ./src/st_koks_uvc_factory.cpp
                               ./src/st_koks_video.cpp
                               ./src/st_koks_audio.cpp
                               ./src/jpeg_encoder.cpp)
endif()

target_include_directories(media_core PRIVATE ./include
                                              ${ROOT_PATH}/ucamera/include
                                              ${ROOT_PATH}/factory/include
                                              ${ROOT_PATH}/common/include
                                              ${ROOT_PATH}/communication/include)

target_link_libraries(media_core PRIVATE koks_common ${LIB_PLATFORM} ucamera)
