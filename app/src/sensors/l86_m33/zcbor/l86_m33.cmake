#
# Generated using zcbor version 0.9.1
# https://github.com/NordicSemiconductor/zcbor
# Generated with a --default-max-qty of 3
#

add_library(l86_m33)
target_sources(l86_m33 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../.venv/lib/python3.12/site-packages/zcbor/src/zcbor_decode.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../.venv/lib/python3.12/site-packages/zcbor/src/zcbor_encode.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../.venv/lib/python3.12/site-packages/zcbor/src/zcbor_common.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../.venv/lib/python3.12/site-packages/zcbor/src/zcbor_print.c
    ${CMAKE_CURRENT_LIST_DIR}/l86_m33_encode.c
    )
target_include_directories(l86_m33 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../../.venv/lib/python3.12/site-packages/zcbor/include
    ${CMAKE_CURRENT_LIST_DIR}
    )
