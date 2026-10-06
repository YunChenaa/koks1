add_library(LCD STATIC ./src/h264decoder.cpp
                      ./src/LCD.cpp 
                      ./src/LCDVedioPlayer.cpp
                      ./src/draw_text_nv12.cpp
                      ./src/cn_font_48_2bpp.cpp
                      ./src/text_motion_track.cpp
                      ./src/LCDCameraPlayer.cpp
                      )

target_include_directories(LCD PRIVATE ./include
                                              ${ROOT_PATH}/common/include
                                              ${ROOT_PATH}/db/include
                                              ${ROOT_PATH}/communication/include
                                              ${ROOT_PATH}/media_core/include
                                              ${ROOT_PATH}/ucamera/include
                                              )

target_link_libraries(LCD PRIVATE koks_common ${LIB_OPEN264} koks_db communication media_core ucamera)


