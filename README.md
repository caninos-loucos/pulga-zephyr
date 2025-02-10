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
until just before getting Zephyr source code (before `west init` command, on Step 4 of `Get Zephyr and install Python dependencies` section).

### Initialization

The first step is to initialize the workspace folder (`my-workspace`) where
the `pulga-zephyr` repository and all Zephyr modules will be cloned. Run the following
command:

```shell
# initialize my-workspace for pulga-zephyr (main branch)
west init -m https://github.com/caninos-loucos/pulga-zephyr --mr main my-workspace
# update Zephyr modules
cd my-workspace
west update
```

After running this commands, you can continue the guide until the end of `Install the Zephyr SDK` section.

### Building and running

To build the application, run the following command:

```shell
cd pulga-zephyr
west build -b $BOARD app
```

where `$BOARD` is the target board, `pulga`.
If you have problems building the application - and specially after changing application or compilation configuration options -, try running a pristine build
by appending `-p` to the last command. There are also other examples in the
`samples` folder, that can be built providing by the proper directories instead
of `app`.

A sample debug configuration is also provided. To apply it, run the following
command:

```shell
west build -b $BOARD app -- -DOVERLAY_CONFIG=debug.conf
```

Once you have built the application, run the following command to flash it or use J-Flash Lite.

```shell
west flash
```

### Additional features

Features that are external to the Pulga Core board, such as SCD30 sensor, GPS sampling and LoraWAN, need to activated by uncommenting the respective pieces of code in `app/CMakeLists.txt`. For example, if you want to activate GNSS (GPS) sensoring, the following line needs to be uncommented:

```
list(APPEND SHIELD pulga_gps)
```

## Configuring the application

To deactivate sensors internal to Pulga Core, you simply need to change their status from "okay" to "disabled" in `app/boards/pulga.overlay`. The sensors sampling interval, the transmission interval of the communication channels, the size of the ring buffer and the use of UART communication to terminal are to be configured in `app/prj.conf` and the description of those options can be found in `app/Kconfig`.

### Application configurations

- `SAMPLING_INTERVAL`: periodically, after the configured time in milliseconds, all activated sensors will write measurements to the ring buffer.

  - Constraints:
    - SCD30: 2s < t < 180s.
    - L86 GNSS module: 100ms < t < 10s. If t > 1000ms, it needs to be a multiple of 1000. If the interval is bigger than 10s, the application will ignore GNSS measurements and only allow writing to buffer in the configured time.

- `TRANSMISSION_INTERVAL`: periodically, after the configured time in milliseconds, a thread will read an item from the buffer and wake all activated communications channels to transmit it.

  - Constraints:
    - LoRaWAN: transmission takes about 5s, so if the transmission interval is too large and the sampling period is too low, data might be lost. To prevent this, this application uses another buffer, internal to LoRaWAN.

- `BUFFER_WORDS`: number of 32-bit words the ring buffer can hold, including headers. Every item stored in the buffer has a 32-bit header. When compiling the application, total used RAM predicted by West needs to be less than 99%.

### Communication configurations

- `SEND_UART`: Prints a verbose output to the configured terminal, such as screen, TeraTerm or MiniCOM.
- `SEND_LORA`: Sends a compressed version of the output via LoRaWAN, requiring pulga-lora shield to be activated.

#### LoRaWAN configurations

- `LORAWAN_DR`: Datarate used in LoRaWAN communication. This affects several communication parameters. The lower the datarate, the smaller the maximum payload size, the lower the range, the slower the communication and the higher the power consumption.
- `LORAWAN_ACTIVATION`: Whether joining the LoRaWAN network will be via OTAA (more secure, renews encryption keys during communication) or ABP (less secure, configures keys to be used during all communication).
- `LORAMAC_REGION_LA915`: The LoRaWAN region affects parameters such as the bandwidth, the number of channels, etc. The default region for American Tower apllications is LA915, but other providers may use AU915 in Brazil.

##### LoRaWAN usage

The LoRaWAN stack requires the modification of some parameters in other files, according to the specific end application:

- LoRaWAN Region (`app/src/communication/lorawan/lorawan_interface.c`): `LORAWAN_SELECTED_REGION` must match the region selected by the `LORAMAC_REGION` configuration mentioned in the previous section.
- LoRaWAN parameters (`app/src/communication/lorawan/lorawan_keys_example.h`): The referenced file contains an example of keys used for LoRaWAN communication. In production environment, it's necessary to create a new `lorawan_keys.h` file, which will be properly ignored by git, and include it in the `lorawan_interface.c` file. Changes to `lorawan_keys_example.h` MUST NOT be committed to the repository. 
- Power amplifier output pin (`boards/shields/pulga-lora/pulga-lora.overlay`): On the lora0 (sx1272) node, one may need to change the `power-amplifier-output` property according to the type of Pulga Lora board used. Types A and B don't have PA boost, so "rfo" pin is used, while types C and D use "pa-boost" pin.

## Data encoding

Different applications and interfaces require data to be encoded in different formats, whether to save storage space or transmission time, to facilitate debugging, or to meet the format required by an external API. The currently supported encoding types can be be found in `EncodingLevel` enumeration in `app/src/data_processing/data_abstraction.h`. Each communication channel can define which encoding type it will use to transmit data.

### Verbose 

Default encoding format for UART communication with terminal. Adequate for cases in which the number of transmitted bytes isn't a limitation. Provides the most complete and naturally comprehensible description of the data.

```shell
"Temperature: 25.08 oC; Pressure: 968.0 hPa; Humidity: 71 %RH;"
```

### Minimalist

This encoding format reduces the number of stored or transmitted bytes while still maintaining the representation of the data in text. It's adequate for debugging new features in LoRaWAN communication without the need to parse and process the transmitted bytes in the receptor. The specifications of what types of measurements correspond to each symbol in the minimalist encoded messages are described in `app/src/sensors/XXX/XXX_model.c` files.

```shell
"T2508P968H71"
```

### Raw bytes

The application, by default, encodes the data retrieved from the application buffer in binary representation when communicating via LoRaWAN. In this encoding format, the structs that represent the sensors readings are preceded by a byte that corresponds to the identification of the type of data that follows. For example, for the BME280 sensor readings, one data item may be represented in hexadecimal as:

```shell
0x00 0x0a 0x08 0x25 0xd0 0x47
```

The first byte of a data item is always the identifier. This way we'll know what are we looking at when we look at the rest of the data. In this example, the first byte is `0x00`, which corresponds to the data returned by BME280 temperature, pressure and humidity sensor. The list of data types is defined by `DataType` enumeration in `app/src/data_processing/data_abstraction.h`.

The struct corresponding to the BME280 sensor readings is implemented as follows in the source file `app/src/sensors/bme280/bme280_service.h`:

```code
typedef struct
{
    int16_t temperature;
    uint16_t pressure;
    uint8_t humidity;
} SensorModelBME280;
```

Therefore, when we look at the binary data, we will first read a signed 16-bit integer, then an unsigned 16-bit integer and then a byte. `0x0a 0x08` is a signed int for the temperature (2508, which means 25.08°C), `0x25d0` is an unsigned int that represents the pressure (9680, which is 968.0 hPa) and 0x47 is a byte that represents the humidity (71, corresponding to 71%).

Other sensors will have their respective structs documented in their `app/src/sensors/XXX/XXX_service.h` files.

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
