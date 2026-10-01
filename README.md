# SerialProjectorLink

SerialProjectorLink connects an Epson projector to your network through its RS-232C serial port, using an ESP8266 board.
It is based on [Serial2Net ESP8266](https://github.com/soif/Serial2Net_ESP8266).

Tested with Home Assistant ([Epson integration](https://www.home-assistant.io/integrations/epson/)) and an Epson EH-TW7100. Other Epson projectors that support ESC/VP21 over RS-232C should work as well.

## How it works

Network-enabled Epson projectors are controlled over TCP using ESC/VP.net. A client (e.g. Home Assistant) first sends an ESC/VP.net handshake, then plain ESC/VP21 commands such as `PWR ON` or `PWR?`.

The serial port of the projector only understands ESC/VP21. The ESP8266 therefore:

- answers the ESC/VP.net handshake itself, without forwarding it to the projector,
- forwards all other commands to the projector's serial port,
- sends the projector's responses back to all connected clients.

## Features

- Makes Epson projectors without (or with unused) LAN port controllable over WiFi
- Supports up to 4 simultaneous TCP clients; when all are in use, the oldest connection is replaced
- Dead connections are detected via TCP keepalive
- Static IP or DHCP
- Optional watchdog (`USE_WDT`)

## Configuration

Edit `Serial2Net_ESP8266/config.default.h` before uploading the sketch:

| Setting | Default | Description |
|---|---|---|
| `STATIC_IP` | enabled | Comment out to use DHCP |
| `IP_ADDRESS`, `GATEWAY_ADDRESS`, `NET_MASK` | `10.1.7.41`, `10.1.11.1`, `255.255.0.0` | Network settings for static IP |
| `WIFI_SSID`, `WIFI_PASSWORD` | placeholders | Your WiFi credentials |
| `TCP_LISTEN_PORT` | `9999` | TCP port the clients connect to |
| `BAUD_RATE` | `57600` | Serial speed, must match the serial speed of your projector |
| `USE_WDT` | disabled | Enable the watchdog |

Then flash `Serial2Net_ESP8266/Serial2Net_ESP8266.ino` with the Arduino IDE. The ESP8266 board package (core) version 3.0 or newer is required. No additional libraries are needed.

## Wiring

The ESP8266 uses 3.3 V TTL levels, the projector uses RS-232 levels. Connect them through an RS-232 level converter (e.g. a MAX3232 module):

| ESP8266 | Level converter | Projector (D-Sub 9) |
|---|---|---|
| TX | TTL RX → RS-232 TX | RxD (pin 2) |
| RX | TTL TX ← RS-232 RX | TxD (pin 3) |
| GND | GND | GND (pin 5) |
| 3.3 V | VCC | – |

Depending on your converter module, TX/RX may need to be swapped. On some projectors RS-232C has to be selected as control port in the projector menu.

## Home Assistant

Add the Epson integration and enter the IP address of the ESP8266 as host.

## License

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
