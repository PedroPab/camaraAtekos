#ifndef WIFI_H
#define WIFI_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// Incluir archivos HTML
#include "html/index_html.h"
#include "html/404_html.h"
#include "html/credentials_saved_html.h"

extern Preferences preferences;
extern WebServer server;
extern String ssid;
extern String password;

const char* ssid_ap = "ESP32_AP";
const char* password_ap = "12345678";

void saveCredentials(String ssid, String password) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
}

void loadCredentials() {
  preferences.begin("wifi", true);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Connecting to WiFi with SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) { // Incrementado a 40 intentos
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Connected to the WiFi network");
    initCamera(); // Iniciar la cámara al conectarse al WiFi
  } else {
    Serial.println();
    Serial.println("Failed to connect. Starting AP mode.");
    startAPMode();
  }
}

void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid_ap, password_ap);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot() {
  String html = index_html;
  server.send(200, "text/html", html);
}

void handleSubmit() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    saveCredentials(ssid, password);
    server.send(200, "text/html", credentials_saved_html);
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/html", "Invalid Request");
  }
}

void handleNotFound() {
  server.send(404, "text/html", not_found_html);
}

#endif // WIFI_H
