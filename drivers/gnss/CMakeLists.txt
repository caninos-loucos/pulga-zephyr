# Copyright (c) 2024 LSI-TEC - Matheus de Almeida Orsi e Silva
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers/gnss)
zephyr_library_sources_ifdef(CONFIG_GNSS_QUECTEL_L86 gnss_quectel_l86.c)