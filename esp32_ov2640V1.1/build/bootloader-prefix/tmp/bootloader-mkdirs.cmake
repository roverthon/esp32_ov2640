# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/ESP32/Espressif/frameworks/esp-idf-v5.1.2/components/bootloader/subproject"
  "C:/Users/Z8866/Desktop/esp32_ov2640/build/bootloader"
  "C:/Users/Z8866/Desktop/esp32_ov2640/build/bootloader-prefix"
  "C:/Users/Z8866/Desktop/esp32_ov2640/build/bootloader-prefix/tmp"
  "C:/Users/Z8866/Desktop/esp32_ov2640/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/Z8866/Desktop/esp32_ov2640/build/bootloader-prefix/src"
  "C:/Users/Z8866/Desktop/esp32_ov2640/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Z8866/Desktop/esp32_ov2640/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Z8866/Desktop/esp32_ov2640/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
