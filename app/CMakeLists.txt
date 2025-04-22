#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# Uncomment to activate GPS shield, when Pulga-GPS module is attached to the stack
# list(APPEND SHIELD pulga_gps)

# Uncomment to activate LoRa shield, when Pulga-LoRa module is attached to the stack
list(APPEND SHIELD pulga_lora)

# Uncomment to activate SCD30 shield, a CO2 concentration sensor
# list(APPEND SHIELD scd30)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_sources(app PRIVATE 
                    src/main.c
                    src/communication/comm_interface.c
                    src/communication/uart/uart_interface.c
                    src/integration/data_buffer/buffer_service.c
                    src/integration/data_abstraction/abstraction_service.c
                    src/integration/data_abstraction/text_model/text_model.c
                    src/sensors/sensors_interface.c
                    src/sensors/bme280/bme280_model.c
                    src/sensors/bme280/bme280_service.c
                    src/sensors/bmi160/bmi160_model.c
                    src/sensors/bmi160/bmi160_service.c
                    src/sensors/si1133/si1133_model.c
                    src/sensors/si1133/si1133_service.c)

if(CONFIG_SHIELD_PULGA_GPS)
    target_sources(app PRIVATE  
                        src/sensors/l86_m33/gnss_model.c
                        src/sensors/l86_m33/l86_m33_service.c)
endif()

if(CONFIG_SEND_LORAWAN)
    target_sources(app PRIVATE  
                        src/communication/lora/lora_common.c
                        src/communication/lora/lora_device/lora_device.c
                        src/communication/lora/lora_device/lorawan_device/lorawan_device.c
                        src/communication/lora/lorawan/lorawan_interface.c
                        src/communication/lora/lorawan/lorawan_setup.c)
endif()

if(CONFIG_SEND_LORA_P2P)
    target_sources(app PRIVATE
                        src/communication/lora/lora_common.c
                        src/communication/lora/lora_device/lora_device.c
                        src/communication/lora/lora_device/lora_p2p_device/lora_p2p_device.c
                        src/communication/lora/lora_p2p/lora_p2p_interface.c)
endif()

if(CONFIG_SHIELD_SCD30)
    target_sources(app PRIVATE  
                        src/sensors/scd30/scd30_model.c
                        src/sensors/scd30/scd30_service.c)
endif()

if(NOT CONFIG_EVENT_TIMESTAMP_NONE)
    target_sources(app PRIVATE  
                        src/integration/timestamp/timestamp_service.c)
endif()

if(CONFIG_SHELL)
    target_sources(app PRIVATE
                        src/communication/shell_commands.c)
endif()
