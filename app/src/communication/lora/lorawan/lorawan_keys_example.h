// IN PRODUCTION, SET THIS FILE NAME TO "lorawan_keys.h" SO IT'S NOT COMMITTED TO GIT

/**
 * OTAA - Over The Air Activation: root keys are used to generate
 *        different session keys during communication - more secure
 */
// 64-bit Device Extended Unique Identifier, required during OTAA to globally identify the device
#define LORAWAN_DEV_EUI {0xca, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01} 
// 64-bit Application Extended Unique Identifier, required to identify the entity which will
// process join requests and with which the device will communicate
#define LORAWAN_APP_EUI {0xca, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
// 128-bit Application Key, used to generate keys for encrypting 
// communication and generating session with OTAA
#define LORAWAN_APP_KEY {0xca, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
// 128-bit Network Key, used to generate keys for signing Message 
// Integrity Codes during join network requests
#define LORAWAN_NET_KEY {0xca, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}

/**
 * ABP - Activation By Personalization: devices use fixed address
 *       and session key during all communication - less secure
 */
// 32-bit Device Address, required during ABP to identify the device within the network
// or dynamically generated after joining the network with OTAA
#define LORAWAN_DEV_ADDR 0xcafe0001
