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

The first step is to initialize the workspace folder (``my-workspace``) where
the ``pulga-zephyr`` repository and all Zephyr modules will be cloned. Run the following
command:

```shell
# initialize my-workspace for pulga-zephyr (main branch)
west init -m https://github.com/caninos-loucos/pulga-zephyr --mr main my-workspace
# update Zephyr modules
cd my-workspace
west update
```

After running this commands, you can continue the guide until the end of ``Install the Zephyr SDK`` section.

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

Features that are external to the Pulga Core board, such as SCD30 sensor, GPS sampling and LoraWAN, need to activated by uncommenting the respective pieces of code in ``app/CMakeLists.txt``. For example, if you want to activate GNSS (GPS) sensoring, the following line needs to be uncommented:

```
set(SHIELD pulga_gps)
```

### Configuring the application

To deactivate sensors internal to Pulga Core, you simply need to change their status from "okay" to "disabled" in ``app/boards/pulga.overlay``. The sensors sampling interval, the transmission interval of the communication channels, the size of the ring buffer and the use of UART communication to terminal are to be configured in ``app/prj.conf`` and the description of those options can be found in ``app/Kconfig``.

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
