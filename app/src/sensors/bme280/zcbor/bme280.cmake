#
# Generated using zcbor version 0.9.1
# https://github.com/NordicSemiconductor/zcbor
# Generated with a --default-max-qty of 3
#

add_library(bme280)
target_sources(bme280 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../.venv/lib/python3.12/site-packages/zcbor/src/zcbor_decode.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../.venv/lib/python3.12/site-packages/zcbor/src/zcbor_encode.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../.venv/lib/python3.12/site-packages/zcbor/src/zcbor_common.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../.venv/lib/python3.12/site-packages/zcbor/src/zcbor_print.c
    ${CMAKE_CURRENT_LIST_DIR}/bme280_encode.c
    )
target_include_directories(bme280 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../.venv/lib/python3.12/site-packages/zcbor/include
    ${CMAKE_CURRENT_LIST_DIR}
    )
