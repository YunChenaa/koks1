set(COMMON_SOURCES
  ./src/audio_test.cpp
  ./src/factory.cpp
)

if(BUILD_WSSERVER)
  set(EXTRA_SOURCES
    ./src/audio_test_webSocket.cpp
  )
endif()


set(ALL_SOURCES ${COMMON_SOURCES} ${EXTRA_SOURCES})

add_library(factory STATIC ${ALL_SOURCES})

target_include_directories(
  factory PRIVATE ./include
  	${ROOT_PATH}/common/include
    ${ROOT_PATH}/db/include
    ${ROOT_PATH}/otp/include
    ${ROOT_PATH}/rota/include
    ${ROOT_PATH}/media_core/include)

target_link_libraries(
  factory PRIVATE
      koks_common
      media_core
      app_flash_otp
      rota_common
      ${LIB_WSSERVER})
