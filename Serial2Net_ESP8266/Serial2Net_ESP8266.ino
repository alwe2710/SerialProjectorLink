/*
	SerialProjectorLink
	Based on Serial2Net ESP8266 (https://github.com/soif/Serial2Net_ESP8266)
	Copyright 2017 François Déchery

	** Description **********************************************************
	Bridges the RS-232C port of an Epson projector to/from a (Wifi attached)
	LAN using a ESP8266 board. Answers the ESC/VP.net handshake itself and
	forwards ESC/VP21 commands to the projector.

	** Inpired by ***********************************************************
	* ESP8266 Ser2net by Daniel Parnell
	https://github.com/dparnell/esp8266-ser2net/blob/master/esp8266_ser2net.ino

	* WiFiTelnetToSerial by Hristo Gochkov.
	https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/WiFiTelnetToSerial/WiFiTelnetToSerial.ino
*/

// Use your Own Config #########################################################
#include "config.default.h"


// Includes ###################################################################
#include <ESP8266WiFi.h>

// Defines #####################################################################
#define MAX_SRV_CLIENTS 4
#define CLIENT_BUFFER_SIZE 40
#define WIFI_CONNECT_TIMEOUT 30000ul  // restart if WiFi is not connected after this time (ms)
#define HANDSHAKE_HEADER_SIZE 16      // size of an ESC/VP.net header
#define HANDSHAKE_EXTRA_HEADER_SIZE 18 // size of each additional ESC/VP.net header (e.g. password)

// Variables ###################################################################
WiFiServer server(TCP_LISTEN_PORT);
WiFiClient serverClients[MAX_SRV_CLIENTS];

// per client buffer collecting a command until its terminating CR
uint8_t clientBuffers[MAX_SRV_CLIENTS][CLIENT_BUFFER_SIZE];
size_t clientBufferLen[MAX_SRV_CLIENTS];
// additional handshake header bytes still to be discarded
size_t clientSkipBytes[MAX_SRV_CLIENTS];

// ESC/VP.net handshake: "ESC/VP.net", version 0x10, type CONNECT (0x03)
const uint8_t handshake_request[] = {0x45, 0x53, 0x43, 0x2F, 0x56, 0x50, 0x2E, 0x6E, 0x65, 0x74, 0x10, 0x03, 0x00, 0x00, 0x00};
// Same header with status 0x20 (OK) and no additional headers
const uint8_t handshake_response[] = {0x45, 0x53, 0x43, 0x2F, 0x56, 0x50, 0x2E, 0x6E, 0x65, 0x74, 0x10, 0x03, 0x00, 0x00, 0x20, 0x00};


// #############################################################################
// Main ########################################################################
// #############################################################################

// ----------------------------------------------------------------------------
void setup(void) {

#ifdef USE_WDT
  wdt_enable(1000);
#endif

  // Connect to WiFi network
  connect_to_wifi();

  // Start UART
  Serial.begin(BAUD_RATE);

  // Start server
  server.begin();
  server.setNoDelay(true);
}


// ----------------------------------------------------------------------------
void loop(void) {

#ifdef USE_WDT
  wdt_reset();
#endif

  // Check Wifi connection -----------------
  if (WiFi.status() != WL_CONNECTED) {
    // we've lost the connection, so we need to reconnect
    for (byte i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i]) {
        serverClients[i].stop();
      }
    }
    connect_to_wifi();
  }

  // Check if there are any new clients ---------
  uint8_t i;
  if (server.hasClient()) {
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()) {
        if (serverClients[i]) {
          serverClients[i].stop();
        }
        serverClients[i] = server.accept();
        clientBufferLen[i] = 0;
        clientSkipBytes[i] = 0;
        break;
      }
    }
    // No free/disconnected spot so reject
    if (i == MAX_SRV_CLIENTS) {
      WiFiClient serverClient = server.accept();
      serverClient.stop();
    }
  }

  // check clients for data ------------------------
  for (i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClients[i] && serverClients[i].connected()) {
      while (serverClients[i].available()) {
        handle_client_byte(i, serverClients[i].read());
      }
    }
  }

  // check UART for data --------------------------
  if (Serial.available()) {
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    //push UART data to all connected clients
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i] && serverClients[i].connected()) {
        serverClients[i].write(sbuf, len);
        delay(1);
      }
    }
  }
}


// Functions ###################################################################

// ----------------------------------------------------------------------------
// Collects the data of a client until a command is complete:
// - an ESC/VP.net handshake is answered without communicating to the projector
// - a command terminated by CR (or LF) is passed to the projector with a single CR
void handle_client_byte(uint8_t client, uint8_t c) {
  uint8_t *buf = clientBuffers[client];
  size_t &len = clientBufferLen[client];

  // discard additional handshake headers
  if (clientSkipBytes[client] > 0) {
    clientSkipBytes[client]--;
    return;
  }

  if (c == '\r' || c == '\n') {
    // skip empty commands (e.g. the LF of a CR LF)
    if (len > 0) {
      Serial.write(buf, len);
      Serial.write('\r');
      len = 0;
    }
    return;
  }

  buf[len++] = c;

  // check for a handshake request
  if (len == HANDSHAKE_HEADER_SIZE && memcmp(buf, handshake_request, sizeof(handshake_request)) == 0) {
    serverClients[client].write(handshake_response, sizeof(handshake_response));
    clientSkipBytes[client] = buf[HANDSHAKE_HEADER_SIZE - 1] * HANDSHAKE_EXTRA_HEADER_SIZE;
    len = 0;
    return;
  }

  // buffer full without terminator: pass data to projector as is
  if (len == CLIENT_BUFFER_SIZE) {
    Serial.write(buf, len);
    len = 0;
  }
}


// ----------------------------------------------------------------------------
void connect_to_wifi() {

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

#ifdef STATIC_IP
  IPAddress ip_address = parse_ip_address(IP_ADDRESS);
  IPAddress gateway_address = parse_ip_address(GATEWAY_ADDRESS);
  IPAddress netmask = parse_ip_address(NET_MASK);
  WiFi.config(ip_address, gateway_address, netmask);
#endif

  // connect
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait for WIFI connection, restart if it takes too long
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
#ifdef USE_WDT
    wdt_reset();
#endif
    if (millis() - start > WIFI_CONNECT_TIMEOUT) {
      ESP.restart();
    }
    delay(100);
  }
}


// ----------------------------------------------------------------------------
IPAddress parse_ip_address(const char *str) {
  IPAddress result;
  int index = 0;
  result[0] = 0;
  while (*str) {
    if (isdigit((unsigned char)*str)) {
      result[index] *= 10;
      result[index] += *str - '0';
    } else {
      index++;
      if (index < 4) {
        result[index] = 0;
      }
    }
    str++;
  }
  return result;
}
