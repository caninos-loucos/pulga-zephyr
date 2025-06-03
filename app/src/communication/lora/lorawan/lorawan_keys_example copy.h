// IN PRODUCTION, SET THIS FILE NAME TO "lorawan_keys.h" SO IT'S NOT COMMITTED TO GIT

/**
 * OTAA - Over The Air Activation: root keys are used to generate
 *        different session keys during communication - more secure
 */
// 64-bit Device Extended Unique Identifier, required during OTAA to globally identify the device
#define LORAWAN_DEV_EUI {0x0a, 0x70, 0xfd, 0x77, 0xd8, 0x52, 0xbb, 0x4c}
// 64-bit Application Extended Unique Identifier, required to identify the entity which will
// process join requests and with which the device will communicate
#define LORAWAN_APP_EUI {0x1d, 0x31, 0x81, 0x47, 0x16, 0xfe, 0x94, 0xae}
// 128-bit Application Key, used to generate keys for encrypting
// communication and generating session with OTAA
#define LORAWAN_APP_KEY {0xdc, 0x32, 0xac, 0x1d, 0x41, 0x4a, 0x93, 0xf9, 0x77, 0x25, 0x14, 0x76, 0xce, 0x8b, 0x79, 0x2b}
// 128-bit Network Key, used to generate keys for signing Message
// Integrity Codes during join network requests
#define LORAWAN_NET_KEY {0x18, 0x50, 0xaf, 0x1f, 0x69, 0x58, 0x11, 0x99, 0x40, 0x3b, 0x99, 0x2e, 0x63, 0xcb, 0x06, 0x63}
/**
 * ABP - Activation By Personalization: devices use fixed address
 *       and session key during all communication - less secure
 */
// 32-bit Device Address, required during ABP to identify the device within the network
// or dynamically generated after joining the network with OTAA
#define LORAWAN_DEV_ADDR 0xdd0dbd15
