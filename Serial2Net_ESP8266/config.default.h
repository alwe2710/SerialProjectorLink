/*
    Configuration example
    Rename me to "config_CUSTOM.h"
    and include me from  Serial2Net_ESP8266.ino
*/

//#define USE_WDT

// use GPIO15 (TX) / GPIO13 (RX) instead of GPIO1 (TX) / GPIO3 (RX) for the projector,
// so the boot messages of the ESP8266 are not sent to the projector (requires rewiring)
//#define SWAP_UART

// IP Address ------------------------------------------------------------------
#define STATIC_IP   // comment  to enable DHCP

#ifdef STATIC_IP
#define IP_ADDRESS      "10.1.7.41"
#define GATEWAY_ADDRESS "10.1.11.1"
#define NET_MASK        "255.255.0.0"
#endif

// Wifi credentials ------------------------------------------------------------
#define WIFI_SSID       "SSID_NAME"
#define WIFI_PASSWORD   "Your_SSID_Password"

// Server / Client Settings ----------------------------------------------------
#define TCP_LISTEN_PORT 9999
#define BAUD_RATE       57600   //RFLink default speed
