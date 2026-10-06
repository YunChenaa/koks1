add_library(
  koks_common STATIC ./src/json11.cpp ./src/sys_logger.cpp ./src/os_utils.cpp
                     ./src/soft_codec.cpp ./src/st_koks_led.cpp ./src/partition_guard.cpp ./src/config_bk.cpp)

target_include_directories(koks_common PRIVATE ./include)

target_link_libraries(koks_common PRIVATE)
