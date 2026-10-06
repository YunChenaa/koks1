# Install script for directory: /home/songwei/koks1/application

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/songwei/koks1/application/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/opt/Ingenic/T33/mips-gcc540-glibc222-r3.3.7.mxu2.cve2/bin/mips-linux-uclibc-gnu-objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/bin/koks;/home/songwei/koks1/application/install/bin/rota")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/bin" TYPE PROGRAM FILES
    "/home/songwei/koks1/application/build/entry/koks"
    "/home/songwei/koks1/application/build/rota/rota"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/libs/" TYPE DIRECTORY FILES
    "/home/songwei/koks1/application/build/media_core/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/platform/lib/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/libturbojpeg/lib/"
    FILES_MATCHING REGEX "/[^/]*\\.so[^/]*$" REGEX "/[^/]*CMakeFiles[^/]*$" EXCLUDE)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/bin/koks;/home/songwei/koks1/application/install/bin/rota")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/bin" TYPE PROGRAM FILES
    "/home/songwei/koks1/application/build/entry/koks"
    "/home/songwei/koks1/application/build/rota/rota"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/libs/" TYPE DIRECTORY FILES
    "/home/songwei/koks1/application/build/communication/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/platform/lib/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/libturbojpeg/lib/"
    FILES_MATCHING REGEX "/[^/]*\\.so[^/]*$" REGEX "/[^/]*CMakeFiles[^/]*$" EXCLUDE)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/bin/koks;/home/songwei/koks1/application/install/bin/rota")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/bin" TYPE PROGRAM FILES
    "/home/songwei/koks1/application/build/entry/koks"
    "/home/songwei/koks1/application/build/rota/rota"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/libs/" TYPE DIRECTORY FILES
    "/home/songwei/koks1/application/build/common/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/platform/lib/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/libturbojpeg/lib/"
    FILES_MATCHING REGEX "/[^/]*\\.so[^/]*$" REGEX "/[^/]*CMakeFiles[^/]*$" EXCLUDE)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/bin/koks;/home/songwei/koks1/application/install/bin/rota")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/bin" TYPE PROGRAM FILES
    "/home/songwei/koks1/application/build/entry/koks"
    "/home/songwei/koks1/application/build/rota/rota"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/libs/" TYPE DIRECTORY FILES
    "/home/songwei/koks1/application/build/entry/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/platform/lib/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/libturbojpeg/lib/"
    FILES_MATCHING REGEX "/[^/]*\\.so[^/]*$" REGEX "/[^/]*CMakeFiles[^/]*$" EXCLUDE)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/bin/koks;/home/songwei/koks1/application/install/bin/rota")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/bin" TYPE PROGRAM FILES
    "/home/songwei/koks1/application/build/entry/koks"
    "/home/songwei/koks1/application/build/rota/rota"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/libs/" TYPE DIRECTORY FILES
    "/home/songwei/koks1/application/build/db/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/platform/lib/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/libturbojpeg/lib/"
    FILES_MATCHING REGEX "/[^/]*\\.so[^/]*$" REGEX "/[^/]*CMakeFiles[^/]*$" EXCLUDE)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/bin/koks;/home/songwei/koks1/application/install/bin/rota")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/bin" TYPE PROGRAM FILES
    "/home/songwei/koks1/application/build/entry/koks"
    "/home/songwei/koks1/application/build/rota/rota"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/libs/" TYPE DIRECTORY FILES
    "/home/songwei/koks1/application/build/factory/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/platform/lib/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/libturbojpeg/lib/"
    FILES_MATCHING REGEX "/[^/]*\\.so[^/]*$" REGEX "/[^/]*CMakeFiles[^/]*$" EXCLUDE)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/bin/koks;/home/songwei/koks1/application/install/bin/rota")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/bin" TYPE PROGRAM FILES
    "/home/songwei/koks1/application/build/entry/koks"
    "/home/songwei/koks1/application/build/rota/rota"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/libs/" TYPE DIRECTORY FILES
    "/home/songwei/koks1/application/build/otp/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/platform/lib/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/libturbojpeg/lib/"
    FILES_MATCHING REGEX "/[^/]*\\.so[^/]*$" REGEX "/[^/]*CMakeFiles[^/]*$" EXCLUDE)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/bin/koks;/home/songwei/koks1/application/install/bin/rota")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/bin" TYPE PROGRAM FILES
    "/home/songwei/koks1/application/build/entry/koks"
    "/home/songwei/koks1/application/build/rota/rota"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/libs/" TYPE DIRECTORY FILES
    "/home/songwei/koks1/application/build/ucamera/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/platform/lib/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/libturbojpeg/lib/"
    FILES_MATCHING REGEX "/[^/]*\\.so[^/]*$" REGEX "/[^/]*CMakeFiles[^/]*$" EXCLUDE)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/bin/koks;/home/songwei/koks1/application/install/bin/rota")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/bin" TYPE PROGRAM FILES
    "/home/songwei/koks1/application/build/entry/koks"
    "/home/songwei/koks1/application/build/rota/rota"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//;/home/songwei/koks1/application/install/libs//")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/libs/" TYPE DIRECTORY FILES
    "/home/songwei/koks1/application/build/rota/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/platform/lib/"
    "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/libturbojpeg/lib/"
    FILES_MATCHING REGEX "/[^/]*\\.so[^/]*$" REGEX "/[^/]*CMakeFiles[^/]*$" EXCLUDE)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install//config")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/" TYPE DIRECTORY FILES "/home/songwei/koks1/application/config" REGEX "/[^/]*\\.h$" EXCLUDE)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/bin/run.sh;/home/songwei/koks1/application/install/bin/koks_daemon.sh")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/bin" TYPE PROGRAM FILES
    "/home/songwei/koks1/application/script/run.sh"
    "/home/songwei/koks1/application/script/koks_daemon.sh"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/config/db/")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/config/db" TYPE DIRECTORY FILES "/home/songwei/koks1/application/3rd/Ingenic/mips-linux-gnu/facesdk/db/")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/models/alignment_t33_magik.mgk;/home/songwei/koks1/application/install/models/detection_t33_magik.mgk;/home/songwei/koks1/application/install/models/feature_t33_magik.mgk")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/models" TYPE FILE FILES
    "/home/songwei/koks1/application/face-models/alignment_t33_magik.mgk"
    "/home/songwei/koks1/application/face-models/detection_t33_magik.mgk"
    "/home/songwei/koks1/application/face-models/feature_t33_magik.mgk"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/songwei/koks1/application/install/models/live2i_t33_magik.mgk")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/songwei/koks1/application/install/models" TYPE FILE FILES "/home/songwei/koks1/application/face-models/live2i_t33_magik.mgk")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/songwei/koks1/application/build/media_core/cmake_install.cmake")
  include("/home/songwei/koks1/application/build/communication/cmake_install.cmake")
  include("/home/songwei/koks1/application/build/common/cmake_install.cmake")
  include("/home/songwei/koks1/application/build/entry/cmake_install.cmake")
  include("/home/songwei/koks1/application/build/db/cmake_install.cmake")
  include("/home/songwei/koks1/application/build/factory/cmake_install.cmake")
  include("/home/songwei/koks1/application/build/otp/cmake_install.cmake")
  include("/home/songwei/koks1/application/build/ucamera/cmake_install.cmake")
  include("/home/songwei/koks1/application/build/rota/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/songwei/koks1/application/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
