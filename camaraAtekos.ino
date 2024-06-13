#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "esp_camera.h"
#include <HTTPClient.h>

// html
#include "html/index_html.h"             // Incluir el archivo con el HTML
#include "html/404_html.h"               // Incluir el archivo con el HTML 404
#include "html/credentials_saved_html.h" // Incluir el archivo con el HTML de credenciales guardadas

Preferences preferences;

const char *ssid_ap = "ESP32_AP";
const char *password_ap = "12345678";

WebServer server(80);
HTTPClient http;

String ssid;
String password;

unsigned long lastAttemptTime = 0;
const unsigned long attemptInterval = 300000; // 5 minutos

void saveCredentials(String ssid, String password)
{
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
}

void loadCredentials()
{
  preferences.begin("wifi", true);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();
}

void connectToWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Connecting to WiFi with SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println();
    Serial.println("Connected to the WiFi network");
  }
  else
  {
    Serial.println();
    Serial.println("Failed to connect. Starting AP mode.");
    startAPMode();
  }
}

void startAPMode()
{
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

void handleRoot()
{
  String html = index_html;
  server.send(200, "text/html", html);
}

void handleSubmit()
{
  if (server.hasArg("ssid") && server.hasArg("password"))
  {
    ssid = server.arg("ssid");
    password = server.arg("password");
    saveCredentials(ssid, password);

    server.send(200, "text/html", credentials_saved_html);
    delay(1000);
    ESP.restart();
  }
  else
  {
    server.send(400, "text/html", "Invalid Request");
  }
}

// Función para manejar el error 404
void handleNotFound()
{
  server.send(404, "text/html", not_found_html);
}

void setup()
{
  Serial.begin(115200);
  loadCredentials();
  if (ssid == "" || password == "")
  {
    startAPMode();
  }
  else
  {
    connectToWiFi();
  }
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    server.handleClient(); // Manejar las solicitudes del servidor web en modo AP
    if (millis() - lastAttemptTime > attemptInterval)
    {
      lastAttemptTime = millis();
      Serial.println("Retrying to connect to WiFi...");
      connectToWiFi();
    }
    return;
  }
  // Ejecutar funcionalidades adicionales cuando esté conectado al WiFi
  delay(10000); // Esperar 10 segundos antes de capturar y enviar otra foto
  // conecado al wifi
}
