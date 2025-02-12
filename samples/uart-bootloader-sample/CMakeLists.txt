# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# MCUboot related
list(APPEND mcuboot_OVERLAY_CONFIG
  "${CMAKE_CURRENT_SOURCE_DIR}/conf/mcuboot/mcuboot.conf"
)
list(APPEND mcuboot_DTC_OVERLAY_FILE
  "${CMAKE_CURRENT_LIST_DIR}/conf/mcuboot/mcuboot.overlay"
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart-bootloader-sample)

target_sources(app PRIVATE src/main.c)
