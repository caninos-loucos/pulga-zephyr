.. zephyr:code-sample:: lora-send-p2p
   :name: LoRa send
   :relevant-api: lora_api

   Transmit a preconfigured payload every second using the LoRa radio.

Overview
********

This sample demonstrates how to use the LoRa radio driver to configure
the encoding settings and send data over the radio.

Transmitted messages can be received by building and flashing the accompanying
LoRa receive sample :zephyr:code-sample:`lora-receive-p2p` on another board within
range.

Building and Running
********************

Build and flash the sample as follows, changing ``pulga`` for
your board, where your board has a ``lora0`` alias in the devicetree.

.. zephyr-app-commands::
   :zephyr-app: pulga-zephyr/samples/lora-send-p2p
   :host-os: unix
   :board:pulga
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    [00:00:00.531,000] <inf> lora_send: Data sent!
    [00:00:01.828,000] <inf> lora_send: Data sent!
    [00:00:03.125,000] <inf> lora_send: Data sent!
    ...