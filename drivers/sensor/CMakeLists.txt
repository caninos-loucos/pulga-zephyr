# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_SI1133 si1133)
add_subdirectory_ifdef(CONFIG_SCD30 scd30)
add_subdirectory_ifdef(CONFIG_TCS34725 tcs34725)