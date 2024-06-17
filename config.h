// config.h

#ifndef CONFIG_H
#define CONFIG_H

// Define el modo de operación: "PRODUCTION" o "TEST"
// #define MODE_PRODUCTION true

#ifdef MODE_PRODUCTION
  #define WIFI_SSID "CLARO_WIFIC2"
  #define WIFI_PASSWORD "CLAROI06367"
  #define SERVER_URL "serveratekosroom-production.up.railway.app"
  #define ENDPOINT_API_UP_IMG "/api/v1/img"
  #define SERVER_PORT 443
  #define USE_HTTPS true
#else
  #define WIFI_SSID "CLARO_WIFIC2"
  #define WIFI_PASSWORD "CLAROI06367"
  #define SERVER_URL "192.168.20.11"
  #define ENDPOINT_API_UP_IMG "/api/v1/img"
  #define SERVER_PORT 3012
  // #define USE_HTTPS false

#endif

#endif

