.. zephyr:code-sample:: lora-receive-p2p
   :name: LoRa receive p2p
   :relevant-api: lora_api

   Receive packets in asynchronous mode using the LoRa radio.

Overview
********

This sample demonstrates how to use the LoRa radio driver to receive packets
asynchronously.

In order to successfully receive messages, build and flash the accompanying
LoRa send sample :zephyr:code-sample:`lora-send-p2p` on another board within range.

Building and Running
********************

Build and flash the sample as follows, changing ``pulga`` for
your board, where your board has a ``lora0`` alias in the devicetree.

.. zephyr-app-commands::
   :zephyr-app: pulga-zephyr/samples/lora-receive-p2p
   :host-os: unix
   :board: pulga
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    [00:00:00.235,000] <inf> lora_receive: Asynchronous reception
    [00:00:00.956,000] <inf> lora_receive: Received data: helloworld (RSSI:-60dBm, SNR:7dBm)
    [00:00:02.249,000] <inf> lora_receive: Received data: helloworld (RSSI:-57dBm, SNR:9dBm)
    [00:00:03.541,000] <inf> lora_receive: Received data: helloworld (RSSI:-57dBm, SNR:9dBm)
    [00:00:04.834,000] <inf> lora_receive: Received data: helloworld (RSSI:-55dBm, SNR:9dBm)
    [00:00:06.127,000] <inf> lora_receive: Received data: helloworld (RSSI:-55dBm, SNR:9dBm)
    [00:00:07.419,000] <inf> lora_receive: Received data: helloworld (RSSI:-55dBm, SNR:9dBm)
    [00:00:08.712,000] <inf> lora_receive: Received data: helloworld (RSSI:-55dBm, SNR:9dBm)
    [00:00:10.004,000] <inf> lora_receive: Received data: helloworld (RSSI:-55dBm, SNR:9dBm)
    [00:00:11.297,000] <inf> lora_receive: Received data: helloworld (RSSI:-55dBm, SNR:9dBm)
    [00:00:12.590,000] <inf> lora_receive: Received data: helloworld (RSSI:-55dBm, SNR:9dBm)
    [00:00:13.884,000] <inf> lora_receive: Received data: helloworld (RSSI:-55dBm, SNR:9dBm)
    [00:00:15.177,000] <inf> lora_receive: Received data: helloworld (RSSI:-55dBm, SNR:9dBm)
    [00:00:16.470,000] <inf> lora_receive: Received data: helloworld (RSSI:-55dBm, SNR:9dBm)
    [00:00:17.762,000] <inf> lora_receive: Received data: helloworld (RSSI:-55dBm, SNR:9dBm)
    ...
