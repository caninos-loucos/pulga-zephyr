#ifndef LORAWAN_INTERFACE_H
#define LORAWAN_INTERFACE_H

#include <zephyr/lorawan/lorawan.h>
#include <communication/comm_interface.h>

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define LORAWAN_PROCESSING_STACK_SIZE 8192
#define LORAWAN_PROCESSING_PRIORITY 5 /* preemptible */

#define LORAWAN_SEND_THREAD_STACK_SIZE 2048
#define LORAWAN_SEND_THREAD_PRIORITY 5 /* preemptible */

// LoRaWAN datarate defines maximum payload size according to region
#if (CONFIG_LORAWAN_DR == 5)
#define LORAWAN_DR LORAWAN_DR_5
#elif (CONFIG_LORAWAN_DR == 4)
#define LORAWAN_DR LORAWAN_DR_4
#elif (CONFIG_LORAWAN_DR == 3)
#define LORAWAN_DR LORAWAN_DR_3
#elif (CONFIG_LORAWAN_DR == 2)
#define LORAWAN_DR LORAWAN_DR_2
#elif (CONFIG_LORAWAN_DR == 1)
#define LORAWAN_DR LORAWAN_DR_1
#elif (CONFIG_LORAWAN_DR == 0)
#define LORAWAN_DR LORAWAN_DR_0
#else
#error "Datarate between 0 and 5 must be chosen"
#endif

// This is the region for American Tower's gateways, that's totally subject to change in the near future...
// The selected region must also be set in prj.conf so the correct parameters are compiled
#define LORAWAN_SELECTED_REGION LORAWAN_REGION_LA915

// Register lorawan callbacks
ChannelAPI *register_lorawan_callbacks();
// Checks readiness of the LoRa device, configures lorawan backend and timestamp synchronization
int init_lorawan_connection();

#endif /* LORAWAN_INTERFACE_H */