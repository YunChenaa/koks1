set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR mips32r2)

set(_CMAKE_TOOLCHAIN_LOCATION  /opt/Ingenic/T33/mips-gcc540-glibc222-r3.3.7.mxu2.cve2/bin)
# set(_CMAKE_TOOLCHAIN_PREFIX mips-linux-gnu-)
set(_CMAKE_TOOLCHAIN_PREFIX mips-linux-uclibc-gnu-)


# set(_CMAKE_TOOLCHAIN_LOCATION  /opt/gcc-linaro-7.3.1_aarch64-linux-gnu/bin)
# set(_CMAKE_TOOLCHAIN_PREFIX aarch64-linux-gnu-)
set(CMAKE_C_COMPILER "${_CMAKE_TOOLCHAIN_LOCATION}/${_CMAKE_TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${_CMAKE_TOOLCHAIN_LOCATION}/${_CMAKE_TOOLCHAIN_PREFIX}g++")
set(CMAKE_AR "${_CMAKE_TOOLCHAIN_LOCATION}/${_CMAKE_TOOLCHAIN_PREFIX}ar")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flax-vector-conversions -D__NODIR_FILE__='$(notdir $<)' -fPIC -muclibc -D_GLIBCXX_USE_C99 -mmxu2")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flax-vector-conversions -D__NODIR_FILE__='$(notdir $<)'  -fPIC -muclibc -D_GLIBCXX_USE_C99 -mmxu2")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-psabi  -Wno-attributes" )
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi  -Wno-attributes")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wdeprecated-declarations -Wno-deprecated-declarations")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdeprecated-declarations -Wno-deprecated-declarations")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -muclibc")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)