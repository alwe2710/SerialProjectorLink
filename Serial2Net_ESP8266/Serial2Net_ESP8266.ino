/*
	SerialProjectorLink
	Based on Serial2Net ESP8266 (https://github.com/soif/Serial2Net_ESP8266)
	Copyright 2017 François Déchery

	** Description **********************************************************
	Briges a Serial Port to/from a (Wifi attached) LAN using a ESP8266 board

	** Inpired by ***********************************************************
	* ESP8266 Ser2net by Daniel Parnell
	https://github.com/dparnell/esp8266-ser2net/blob/master/esp8266_ser2net.ino

	* WiFiTelnetToSerial by Hristo Gochkov.
	https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/WiFiTelnetToSerial/WiFiTelnetToSerial.ino
*/

// Use your Own Config #########################################################
#include "config.default.h"
//#include	"config_315.h"
//#include	"config_433.h"


// Includes ###################################################################
#include <ESP8266WiFi.h>

// Defines #####################################################################
#define MAX_SRV_CLIENTS 4

// Variables ###################################################################
int last_srv_clients_count = 0;
int length_hex;

WiFiServer server(TCP_LISTEN_PORT);
WiFiClient serverClients[MAX_SRV_CLIENTS];


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
        serverClients[i] = server.available();
        //Serial1.print("New client: "); Serial1.print(i);
        continue;
      }
    }
    // No free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }

  // check clients for data ------------------------
  for (i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClients[i] && serverClients[i].connected()) {
      if (serverClients[i].available()) {
        //get data from the telnet client and push it to the UART
        uint8_t buf[40];
        while (serverClients[i].available()) { //should be raplacable by the for-loop below, not tested however
        	buf[length_hex] = serverClients[i].read(); //write input in buffer array
        	length_hex++; 
        }
	//for (length_hex = 0; serverClients[i].available(); length_hex++) {
		//buf[length_hex] = serverClients[i].read(); //write input in buffer array
	//}
        uint8_t init_buf[] = {0x45, 0x53, 0x43, 0x2F, 0x56, 0x50, 0x2E, 0x6E, 0x65, 0x74, 0x10, 0x03, 0x00, 0x00, 0x00}; //Handshake Data to send
        int diff;
        int errors_diff = 0;
        for (int i=0; i<15; i++) {
          diff = buf[i]-init_buf[i];
          if(diff!=0) {
            errors_diff++;
          }
        }
        if(errors_diff == 0) { //check for a Handshake request and answer without communicating to the projector
          byte message[] = {0x45, 0x53, 0x43, 0x2F, 0x56, 0x50, 0x2E, 0x6E, 0x65, 0x74, 0x10, 0x03, 0x00, 0x00, 0x20, 0x00};
          serverClients[i].write(message, sizeof(message));
        }
        else { //otherwise pass command to projector
          //String buf_hex;
          for (int i=0; i<length_hex; i++) { //send text of buffer character by character
            //String buf_str = String(buf[i], HEX);
            //buf_hex += buf_str;
            //Serial.println(buf_str);
            Serial.write(buf[i]);
          }
          Serial.println();
          //Serial.println(buf_hex);
        }
        length_hex = 0;
      }
    }
  }

  // check UART for data --------------------------
  if (Serial.available()) {
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i] && serverClients[i].connected()) {
        //led_rx.pulse();
        serverClients[i].write(sbuf, len);
        //led_tx.update();
        delay(1);
      }
    }
  }
}


// Functions ###################################################################

// ----------------------------------------------------------------------------
void connect_to_wifi() {

  // is this really needed ?
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // connect
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

#ifdef STATIC_IP
  IPAddress ip_address = parse_ip_address(IP_ADDRESS);
  IPAddress gateway_address = parse_ip_address(GATEWAY_ADDRESS);
  IPAddress netmask = parse_ip_address(NET_MASK);
  WiFi.config(ip_address, gateway_address, netmask);
#endif

  // Wait for WIFI connection
  while (WiFi.status() != WL_CONNECTED) {
#ifdef USE_WDT
    wdt_reset();
#endif
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
