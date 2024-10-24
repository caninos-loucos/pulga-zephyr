#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
# Uncomment to activate GPS shield, when Pulga-GPS module is attached to the stack
# set(SHIELD pulga_gps)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_sources(app PRIVATE 
                    src/main.c
                    src/communication/comm_interface.c
                    src/communication/uart/uart_interface.c
                    src/data_processing/data_buffer.c
                    src/data_processing/data_abstraction.c
                    src/sensors/sensors_interface.c
                    src/sensors/bme280/bme280_model.c
                    src/sensors/bme280/bme280_service.c
                    src/sensors/bmi160/bmi160_model.c
                    src/sensors/bmi160/bmi160_service.c
                    src/sensors/si1133/si1133_model.c
                    src/sensors/si1133/si1133_service.c
                    src/sensors/scd30/scd30_model.c
                    src/sensors/scd30/scd30_service.c)

if(CONFIG_SHIELD_PULGA_GPS)
    target_sources(app PRIVATE  
                        src/sensors/l86_m33/gnss_model.c
                        src/sensors/l86_m33/l86_m33_service.c)
endif()
