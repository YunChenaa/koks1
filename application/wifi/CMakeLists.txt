
add_library(koks_wifi STATIC
    ./src/qrcode_scan.cpp
    ./src/aic_wifi.cpp)

target_compile_options(koks_wifi PRIVATE -Wno-write-strings)

target_include_directories(koks_wifi PRIVATE
    ./include
    ${ROOT_PATH}/common/include
    ${ROOT_PATH}/cmcc/include
    ${OVD_PATH}/include
    ${ROOT_PATH}/LCD/include
    ${ROOT_PATH}/media_core/include
    ${ROOT_PATH}/ucamera/include
    ${ROOT_PATH}/rota/include
    ${ROOT_PATH}/factory/include
    )

if(BUILD_CMCC_OVD)
target_link_libraries(koks_wifi PRIVATE koks_common ${LIB_ZBAR} koks_cmcc LCD media_core)
else()
target_link_libraries(koks_wifi PRIVATE koks_common ${LIB_ZBAR} LCD media_core)
endif()

target_link_libraries(koks_wifi PRIVATE rota_update factory)