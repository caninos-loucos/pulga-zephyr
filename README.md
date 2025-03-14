# Pulga Zephyr Generic Application
<!-- 
<a href="https://github.com/zephyrproject-rtos/example-application/actions/workflows/build.yml?query=branch%3Amain">
  <img src="https://github.com/zephyrproject-rtos/example-application/actions/workflows/build.yml/badge.svg?event=push">
</a>
<a href="https://github.com/zephyrproject-rtos/example-application/actions/workflows/docs.yml?query=branch%3Amain">
  <img src="https://github.com/zephyrproject-rtos/example-application/actions/workflows/docs.yml/badge.svg?event=push">
</a>
<a href="https://zephyrproject-rtos.github.io/example-application">
  <img alt="Documentation" src="https://img.shields.io/badge/documentation-3D578C?logo=sphinx&logoColor=white">
</a>
<a href="https://zephyrproject-rtos.github.io/example-application/doxygen">
  <img alt="API Documentation" src="https://img.shields.io/badge/API-documentation-3D578C?logo=c&logoColor=white">
</a> -->

This repository contains a generic application using the Pulga board, 
developed by Caninos Loucos. This example aims to use all sensors and 
communication protocols supported by Pulga. The main purpose of this
repository, based on Zephyr's own [example-application][example_app] on West T2 topology, 
is to serve as a reference on how to structure Zephyr-based applications 
on Pulga. It's recommended to consult the example application before 
developing new features on Pulga to understand how Zephyr works. The 
following features are to be demonstrated in this example:

- Si1133 sensor (luminosity, UV)
- BME280 sensor (umidity, temperature, pressure)
- BMI160 sensor (accelerometer, gyroscope)
- SCD30 sensor - external to the board (CO2)
- Pulga GPS - Pulga shield containing L86 GNSS modem
- LoRaWAN and LoRaWAN peer-to-peer
- Bluetooth
- Using a ring buffer to store sensor data

[example_app]: https://github.com/caninos-loucos/pulga-zephyr

## Getting Started

Before getting started, make sure you have a proper Zephyr development
environment. Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html)
until just before getting Zephyr source code (before ``west init`` command, on Step 4 of ``Get Zephyr and install Python dependencies`` section).

### Initialization

The first step is to initialize the workspace folder (``zephyrproject``) where
the ``pulga-zephyr`` repository and all Zephyr modules will be cloned. Run the following
command:

```shell
# initialize zephyrproject for pulga-zephyr (main branch)
west init -m https://github.com/caninos-loucos/pulga-zephyr --mr main zephyrproject
# update Zephyr modules
cd zephyrproject
west update
```

After running this commands, CONTINUE THE GETTING STARTED GUIDE until the end of ``Install the Zephyr SDK`` section.

### Building and running

To build the application, run the following command:

```shell
cd pulga-zephyr
west build -b pulga app
```

If you have problems building the application - and specially after changing application or compilation configuration options -, try running a pristine build 
by appending `-p` to the last command. There are also other examples in the
`samples` folder, that can be built providing by the proper directories instead
of `app`.

A sample debug configuration is also provided. To apply it, run the following
command:

```shell
west build -b pulga app -- -DOVERLAY_CONFIG=debug.conf
```

Once you have built the application, run the following command to flash it or use J-Flash Lite. When using west, it's necessary to install NRF Command Line Tools beforehand.

```shell
west flash
```

### Additional features

Features that are external to the Pulga Core board, such as SCD30 sensor, GPS sampling and LoraWAN, need to activated by uncommenting the respective lines of code in ``app/CMakeLists.txt``. For example, if you want to activate GNSS (GPS) sensoring, the following line needs to be uncommented:

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
- TRANSMISSION_INTERVAL: periodically, after the configured time in milliseconds, a thread will read an item from the buffer and wake all activated communications channels to transmit it, repeating this until the buffer is empty.
  - Constraints: 
    - LoRaWAN: transmission takes about 5s, so if the transmission interval is too large and the sampling period is too low, data might be lost. To prevent this, this application uses another buffer, internal to LoRaWAN.
- BUFFER_WORDS: number of 32-bit words the ring buffer can hold, including headers. Every item stored in the buffer has a 32-bit header. When compiling the application, total used RAM predicted by West needs to be less than 99%.
- EVENT_TIMESTAMP_SOURCE: this option allows the user to choose whether the application will timestamp the sampling events or not. In case it does, it's possible to configure the source of the time reference between the LoRaWAN network, GNSS satellite data or system uptime. As a choice configuration (available options found in KConfig file), selecting one option will automatically set all others to false.
  - Constraints:
    - EVENT_TIMESTAMP_LORAWAN: this option can only be set when using pulga-lora shield and if LoRaWAN is active.
    - EVENT_TIMESTAMP_GNSS: this option can only be set when using pulga_gps shield.

#### Communication configurations
- SEND_UART: Prints a verbose output to the configured terminal, such as TeraTerm or MiniCOM.
- SEND_LORA: Sends a compressed version of the output via LoRaWAN, requiring pulga-lora shield to be activated.

#### LoRaWAN configurations
- LORAWAN_DR: Datarate used in LoRaWAN communication. This affects several communication parameters. The lower the datarate, the smaller the maximum payload size, the lower the range, the slower the communication and the higher the power consumption.
- LORAWAN_ACTIVATION: Whether joining the LoRaWAN network will be via OTAA (more secure, renews encryption keys during communication) or ABP (less secure, configures keys to be used during all communication).
- LORAWAN_SELECTED_REGION (lorawan_interface.h): The LoRaWAN region affects parameters such as the bandwidth, the number of channels, etc.
- Lorawan keys (lorawan_keys_example.h): Security parameters that allow LoRaWAN communication. In production environment, configured in a lorawan_keys.h file, which will be properly ignored by git, being necessary to update the import in lorawan_setup.c.
- Power amplifier output pin (boards/shields/pulga-lora.overlay): depends on the type of Pulga Lora board used. Types A and B don't have PA boost so "rfo" pin is used, while types C and D use "pa-boost" pin.

<!-- ### Testing

To execute Twister integration tests, run the following command:

```shell
west twister -T tests --integration
```

### Documentation

A minimal documentation setup is provided for Doxygen and Sphinx. To build the
documentation first change to the ``doc`` folder:

```shell
cd doc
```

Before continuing, check if you have Doxygen installed. It is recommended to
use the same Doxygen version used in [CI](.github/workflows/docs.yml). To
install Sphinx, make sure you have a Python installation in place and run:

```shell
pip install -r requirements.txt
```

API documentation (Doxygen) can be built using the following command:

```shell
doxygen
```

The output will be stored in the ``_build_doxygen`` folder. Similarly, the
Sphinx documentation (HTML) can be built using the following command:

```shell
make html
```

The output will be stored in the ``_build_sphinx`` folder. You may check for
other output formats other than HTML by running ``make help``. -->
