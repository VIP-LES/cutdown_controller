# Cutdown Controller

## Overview

This Arduino sketch implements the **controller** side of our high-altitude balloon cutdown system. It is responsible for:

- Sending remote **cutdown commands** over LoRa (RFM9x)
- Listening for and parsing **telemetry packets** from the payload
- Displaying decoded telemetry and status updates to a serial terminal

The controller runs on a microcontroller with an attached RFM95 LoRa radio module and communicates over USB serial for operator input and output.

This system was used as the ground station during the Spring 2025 high-altitude balloon test. It complements the onboard payload firmware by offering remote actuation of the nichrome cutdown relays and displaying live sensor data (temperature, pressure, humidity, GPS, etc.).

## Usage

### Commands

Once the system boots, you can type commands directly into the serial monitor (115200 baud). Valid commands:

- `cutdown balloon` – Sends a packet to activate the balloon cutdown relay
- `cutdown parachute` – Sends a packet to activate the parachute cutdown relay

Each command is sent as a structured protobuf message (`BasePacket → CommandPacket`) using the nanopb library.

### Receiving Packets

The controller listens for incoming LoRa packets and decodes them using nanopb. It supports the following packet types:

- **TelemetryPacket** – Displays temperature, humidity, pressure, GPS location, and relay status
- **AckPacket** – Displays confirmation of a received command (with matching nonce)
- **CommandPacket** – Not expected on this side, but logged if received

All received data is printed in a human-readable format to the serial monitor, including a timestamp, callsign, packet type, and relevant data.

## Code Structure

The main logic is organized into the following functions:

- `cutdownBalloon()` / `cutdownParachute()`  
  Build and send a command packet to the payload via LoRa.

- `send_packet()`  
  Encodes a `BasePacket` protobuf message and transmits it using the RFM95 radio.

- `loop()`  
  Handles serial input, checks for incoming LoRa messages, and triggers appropriate actions.

- `parse_radio_data()`  
  Decodes incoming protobuf messages and routes them based on type.

- `pretty_print_packet()`  
  Outputs packet details to the serial monitor.

Each command is associated with a string trigger and a callback in the `Command` struct array, making it easy to add more commands in the future.

## Dependencies

- [RadioHead](http://www.airspayce.com/mikem/arduino/RadioHead/) – For RFM9x radio communication
- [nanopb](https://jpa.kapsi.fi/nanopb/) – For encoding/decoding Protocol Buffers
- `messages.proto` – Shared message definition between controller and payload, stored in cutdown-packets.