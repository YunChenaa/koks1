add_definitions(-DUSE_SOFT_AES_ENCRPT)
add_library(
  communication STATIC
  ./src/uart.cpp
  ./src/crypto.cpp
  ./src/md5sum.c
  ./src/sense_control.cpp
  ./src/senselock.cpp
  ./src/updater.cpp
  ./src/aes.cpp
  ./src/sensor_stream_ctrl.cpp
  ./src/senselock_handler.cpp
  ./src/faceunlock_wrapper.cpp)

target_include_directories(
  communication PRIVATE ./include
                        ${ROOT_PATH}/common/include
                        ${ROOT_PATH}/db/include
                        ${ROOT_PATH}/media_core/include
                        ${ROOT_PATH}/factory/include
                        ${ROOT_PATH}/otp/include
                        ${ROOT_PATH}/rota/include
                        ${ROOT_PATH}/ucamera/include
                        ${ROOT_PATH}/LCD/include
                        
                        )

target_link_libraries(
  communication
  PRIVATE ${LIB_FACESDK}
          ${LIB_PLATFORM}
          koks_common
          media_core
          app_flash_otp
          factory
          rota_common
          rota_update
          LCD
          )
