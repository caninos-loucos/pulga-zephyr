# Pulga Zephyr Generic Application

This repository contains a generic application (inside the `app` directory) for the
Pulga board, developed by Caninos Loucos. This example aims to use all sensors and
communication protocols supported by Pulga.

This repository is based on Zephyr's own
[example-application](https://github.com/zephyrproject-rtos/example-application)
on West T2 topology, and its main purpose is to be a reference for structuring
Zephyr-based applications on Pulga. It is recommended to consult
the example application before developing new features on Pulga
to understand how Zephyr works.

The following features are to be demonstrated in this example:
- Si1133 sensor (luminosity, UV)
- BME280 sensor (umidity, temperature, pressure)
- BMI160 sensor (accelerometer, gyroscope)
- SCD30 sensor - external to the board (CO2)
- Pulga GPS - Pulga shield containing L86 GNSS modem
- LoRaWAN and LoRaWAN peer-to-peer
- Bluetooth
- Storing sensor data using a ring buffer

## Getting Started

To set up a proper Zephyr development environment, according to the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/l,atest/getting_started/index.html),
you should:
  - Install your dependencies (refer to guide).
  - Create a directory for the Zephyr workspace directory:
    > mkdir zephyrproject && cd zephyrproject
  - Create a new python virtual environment and activate it:
    > \# inside the zephyrproject directory\
    > python3 -mvenv .venv && source .venv/bin/activate
  - Install `west`, Zephyr's meta tool:
    > pip install west
  - Initialize the workspace directory (``zephyrproject``), which will contain
    the `pulga-zephyr` repository, and the Zephyr source code:
    > west init -m https://github.com/caninos-loucos/pulga-zephyr --mr main ./\
    > west update
  - Export a Zephyr CMake package for building Zephyr applications:
    > west zephyr-export
  - Install Python dependencies through `west`:
    > west packages pip --install
  - Install the Zephyr SDK:
    > west sdk install

### Building and running

To update the code running on the Pulga, you must build it and flash it:
  - Enter this project's directory:
    > cd pulga-zephyr
  - Build the `app` application:
    > west build -b pulga "app"
  - Flash it using `west` or use J-Flash Lite. When using `west`,
  it's necessary to install **NRF Command Line Tools** beforehand.
    > west flash

If you have issues building the application
(specially after changing application or compilation configuration options),
try running a **pristine** build:
  > west build -p -b pulga "app"

A sample **debug configuration** is provided.
You can use it for building:
  > west build -b pulga "app" -- -DOVERLAY_CONFIG=debug.conf

There are also other example applications in the `samples` directory,
which can be built by using their directories
instead of the `app` directory as argument:
  > west build -b pulga "samples/blinky"

### Additional features

Features that are external to the Pulga Core board, such as SCD30 sensor, GPS sampling and LoraWAN, need to activated by uncommenting the respective pieces of code in ``app/CMakeLists.txt``. For example, if you want to activate GNSS (GPS) sensoring, the following line needs to be uncommented:

```
list(APPEND SHIELD pulga_gps)
```

### Configuring the application

To deactivate sensors internal to Pulga Core, you simply need to change their status from "okay" to "disabled" in ``app/boards/pulga.overlay``. The sensors sampling interval, the transmission interval of the communication channels, the size of the ring buffer and the use of UART communication to terminal are to be configured in ``app/prj.conf`` and the description of those options can be found in ``app/Kconfig``.

#### Application configurations
- SAMPLING_INTERVAL: periodically, after the configured time in milliseconds, all activated sensors will write measurements to the ring buffer.
  - Constraints:
    - SCD30: 2s < t < 180s.
    - L86 GNSS module: 100ms < t < 10s. If t > 1000ms, it needs to be a multiple of 1000. If the interval is bigger than 10s, the application will ignore GNSS measurements and only allow writing to buffer in the configured time.
- TRANSMISSION_INTERVAL: periodically, after the configured time in milliseconds, a thread will read an item from the buffer and wake all activated communications channels to transmit it.
  - Constraints:
    - LoRaWAN: transmission takes about 5s, so if the transmission interval is too large and the sampling period is too low, data might be lost. To prevent this, this application uses another buffer, internal to LoRaWAN.
- BUFFER_WORDS: number of 32-bit words the ring buffer can hold, including headers. Every item stored in the buffer has a 32-bit header. When compiling the application, total used RAM predicted by West needs to be less than 99%.

#### Communication configurations
- SEND_UART: Prints a verbose output to the configured terminal, such as TeraTerm or MiniCOM.
- SEND_LORA: Sends a compressed version of the output via LoRaWAN, requiring pulga-lora shield to be activated.

#### LoRaWAN configurations
- LORAWAN_DR: Datarate used in LoRaWAN communication. This affects several communication parameters. The lower the datarate, the smaller the maximum payload size, the lower the range, the slower the communication and the higher the power consumption.
- LORAWAN_ACTIVATION: Whether joining the LoRaWAN network will be via OTAA (more secure, renews encryption keys during communication) or ABP (less secure, configures keys to be used during all communication).
- LORAWAN_SELECTED_REGION (lorawan_interface.h): The LoRaWAN region affects parameters such as the bandwidth, the number of channels, etc.
- Lorawan keys (lorawan_keys_example.h): Security parameters that allow LoRaWAN communication. In production environment, configured in a lorawan_keys.h file, which will be properly ignored by git, being necessary to update the import in lorawan_setup.c.
- Power amplifier output pin (boards/shields/pulga-lora.overlay): depends on the type of Pulga Lora board used. Types A and B don't have PA boost so "rfo" pin is used, while types C and D use "pa-boost" pin.
