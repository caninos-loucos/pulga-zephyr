/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#ifndef L86_M33_ENCODE_TYPES_H__
#define L86_M33_ENCODE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY 3

struct ZcborPayloadL86_M33 {
	double latitude;
	double longitude;
	int32_t timestamp;
};

#ifdef __cplusplus
}
#endif

#endif /* L86_M33_ENCODE_TYPES_H__ */
