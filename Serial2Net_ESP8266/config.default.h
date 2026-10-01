/*
    Configuration for SerialProjectorLink
    Adjust the values below before uploading the sketch.
*/

//#define USE_WDT

// IP Address ------------------------------------------------------------------
#define STATIC_IP   // comment  to enable DHCP

#ifdef STATIC_IP
#define IP_ADDRESS      "192.168.1.50"
#define GATEWAY_ADDRESS "192.168.1.1"
#define NET_MASK        "255.255.255.0"
#endif

// Wifi credentials ------------------------------------------------------------
#define WIFI_SSID       "SSID_NAME"
#define WIFI_PASSWORD   "Your_SSID_Password"

// Server / Client Settings ----------------------------------------------------
#define TCP_LISTEN_PORT 3629    // Epson ESC/VP.net port
#define BAUD_RATE       9600    // Epson RS-232C speed (8N1)
