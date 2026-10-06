add_library(koks_db STATIC ./src/st_koks_db.cpp)

target_include_directories(
  koks_db
  PRIVATE ./include ${ROOT_PATH}/common/include ${ROOT_PATH}/media_core/include
          ${ROOT_PATH}/communication/include)

target_link_libraries(koks_db PRIVATE koks_common)
