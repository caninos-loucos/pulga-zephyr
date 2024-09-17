#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_sources(app PRIVATE 
                    src/main.c
                    src/communication/comm_interface.c
                    src/communication/uart/uart_interface.c
                    src/data_processing/data_buffer.c
                    src/data_processing/data_abstraction.c
                    src/data_processing/bme280/bme280_model.c
                    src/data_processing/si1133/si1133_model.c
                    src/sensors/sensors_interface.c
                    src/sensors/bme280/bme280_service.c
                    src/sensors/si1133/si1133_service.c)
