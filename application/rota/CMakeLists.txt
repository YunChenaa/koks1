add_library(rota_common STATIC ./src/rota_common.c)
target_include_directories(rota_common PRIVATE ./include)

add_library(rota_update STATIC
            ./src/rota_update.c
            ./src/rota_image.c
            ./src/rota_md5.c)
target_include_directories(rota_update PRIVATE 
                           ./include
                           ${ROOT_PATH}/communication/include
                           ${CMAKE_INSTALL_PREFIX}/include)

if(KOKS_PROJECT STREQUAL "m53d")
    target_include_directories(rota_update PRIVATE 
                               ${OVD_PATH}/include
                               )

    target_sources(rota_update PRIVATE 
                    ./src/ovd_ota.c
                    ./src/httpOTAdownloader.c
                    ./src/wifi_ota_main.c
                    )

    target_link_libraries(rota_update PRIVATE  ${LIB_SSL} pthread)


    add_executable(rota
                   ./src/wifi_ota_main.c)

    target_include_directories(rota PRIVATE
                               ./include
                               ${OVD_PATH}/include)
                               
    target_link_libraries(rota PRIVATE pthread rota_update rota_common)
else()
    add_executable(rota
                   ./src/rota_main.c
                   ./src/rota_uart.c
                   ./src/rota_control.c)
    target_include_directories(rota PRIVATE
                               ./include
                               ${ROOT_PATH}/communication/include
                               ${ROOT_PATH}/ucamera/include)
    target_link_libraries(rota PRIVATE pthread rota_update rota_common ucamera)
endif()


