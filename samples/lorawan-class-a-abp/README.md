# LoRaWAN ABP sample

This sample joins a LoRaWAN network via ABP activation then sends a ```helloworld``` every ten seconds.

 ____

### Keys
For ABP to work, one needs the network and application keys (for encrypted connections) or only the network keys for non-secure (NS) connections. Besides that, the device needs to be assigned a fixed address and `DEV_` and `JOIN_ (or APP_) EUIs`. Such keys must be defined in a `keys.h` file within the `src/` folder, formatted as such:

```
#define LORAWAN_NET_KEY			{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

#define LORAWAN_APP_KEY			{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

#define LORAWAN_DEV_ADDR		0x00000002

#define LORAWAN_DEV_EUI			{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_JOIN_EUI		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
```

### Region
The correct LoRaWAN region must also be defined in the source code, otherwise communication problems may occur due to differences in timings, bands and channels. Refer to your gateway configuration, then choose the matching region in your code using this: (in this example, AU915 region configuration is used)

```#define LORAWAN_SELECTED_REGION LORAWAN_REGION_AU915```

### Then what?
After the correct keys, region, callbacks and activation mode are set, the device will join the network and start sending its message. 

The last argument of the `lorawan_send()` function can take two values: `LORAWAN_MSG_UNCONFIRMED` will tell the gateway that it doesn't need an ACK and is useful for applications where receiving all packets is not critical, since our device will simply keep sending packets. `LORAWAN_MSG_CONFIRMED` will ask the gateway for an ACK and returns -ETIMEDOUT if our device does not receive it in time.