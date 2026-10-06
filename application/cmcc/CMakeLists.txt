set(COMMON_SOURCES
    ./src/ovd.cpp
    ./src/ovd_init.cpp
    ./src/ovd_alarm.cpp
    ./src/ovd_hibernate.cpp
    ./src/RTSPH264FramedSource.cpp
    ./src/RTSPH264VideoServerMediaSubsession.cpp
    ./src/RTSP_main.cpp
    ./src/RTSPAudioFramedSource.cpp
)

if(BUILD_CMCC_VOIP)
    set(EXTRA_SOURCES
        ./src/cmcc_voip_fun.cpp
        ./src/third_party_device_factory.cc
        ./src/third_party_device.cc
        ./src/cmcc_voip_callback.cpp
    )
    set(EXTRA_INCLUDES
        ${ROOT_PATH}/otp/include
    )
    set(EXTRA_LIBS
        app_flash_otp
    )
else()
    set(EXTRA_SOURCES
        ./src/ovd_alarm.cpp
    )
    set(EXTRA_LIBS
        koks_wifi
    )
endif()

set(ALL_SOURCES ${COMMON_SOURCES} ${EXTRA_SOURCES})

add_library(koks_cmcc STATIC ${ALL_SOURCES})

target_compile_options(koks_cmcc PRIVATE -Wno-write-strings -DNO_OPENSSL)

target_include_directories(koks_cmcc PRIVATE
    ./include
    ${ROOT_PATH}/common/include
    ${ROOT_PATH}/ucamera/include
    ${ROOT_PATH}/media_core/include
    ${ROOT_PATH}/wifi/include
    ${ROOT_PATH}/rota/include
    ${OVD_PATH}/include
    ${LIB_LIVE555_PATH}/BasicUsageEnvironment
    ${LIB_LIVE555_PATH}/groupsock
    ${LIB_LIVE555_PATH}/liveMedia
    ${LIB_LIVE555_PATH}/UsageEnvironment
    ${EXTRA_INCLUDES}
)

target_link_libraries(koks_cmcc PRIVATE
    atomic
    koks_common
    media_core
    ucamera
    ${LIB_OVDSDK}
    ${EXTRA_LIBS}
)
