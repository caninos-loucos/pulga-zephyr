#ifndef LORA_P2P_DEVICE_H
#define LORA_P2P_DEVICE_H
#include <zephyr/device.h>
#include <communication/lora/lora_device/lora_device.h>

#ifdef CONFIG_LORA_P2P_CR
#if (CONFIG_LORA_P2P_CR == 5)
#define LORA_CR CR_4_5
#elif (CONFIG_LORA_P2P_CR == 6)
#define LORA_CR CR_4_6
#elif (CONFIG_LORA_P2P_CR == 7)
#define LORA_CR CR_4_7
#elif (CONFIG_LORA_P2P_CR == 8)
#define LORA_CR CR_4_8
#else
#error "Coding Rate between 5 and 8 must be chosen"
#endif
#endif // CONFIG_LORA_P2P_CR

#ifdef CONFIG_LORA_P2P_BW
#if (CONFIG_LORA_P2P_BW == 125)
#define LORA_BW BW_125_KHZ
#elif (CONFIG_LORA_P2P_BW == 250)
#define LORA_BW BW_250_KHZ
#elif (CONFIG_LORA_P2P_BW == 500)
#define LORA_BW BW_500_KHZ
#else
#error "Bandwidth 125, 250 or 500 must be chosen"
#endif
#endif // CONFIG_LORA_P2P_BW

#ifdef CONFIG_LORA_P2P_SF
#if (CONFIG_LORA_P2P_SF == 12)
#define LORA_SF SF_12
#elif (CONFIG_LORA_P2P_SF == 11)
#define LORA_SF SF_11
#elif (CONFIG_LORA_P2P_SF == 10)
#define LORA_SF SF_10
#elif (CONFIG_LORA_P2P_SF == 9)
#define LORA_SF SF_9
#elif (CONFIG_LORA_P2P_SF == 8)
#define LORA_SF SF_8
#elif (CONFIG_LORA_P2P_SF == 7)
#define LORA_SF SF_7
#elif (CONFIG_LORA_P2P_SF == 6)
#define LORA_SF SF_6
#else
#error "Spreading Factor between 6 and 12 must be chosen"
#endif
#endif // CONFIG_LORA_P2P_SF

extern LoraDeviceAPI lora_p2p_device;
// Stops Lora Peer-to-Peer reception
int stop_lora_p2p_reception(const struct device *lora_device);

#endif // LORA_P2P_DEVICE_H