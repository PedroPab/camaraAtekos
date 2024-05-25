#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>

Preferences preferences;

const char* ssid_ap = "ESP32_AP";
const char* password_ap = "12345678";

WebServer server(80);

String ssid;
String password;

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
  Serial.print("Connecting to WiFi ..");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Connected to the WiFi network");
    // Aquí puedes iniciar otras funcionalidades como la cámara
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
  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot() {
  String html = "<html><body><h1>WiFi Config</h1><form action='/submit' method='post'>SSID:<br><input type='text' name='ssid'><br>Password:<br><input type='password' name='password'><br><br><input type='submit' value='Submit'></form></body></html>";
  server.send(200, "text/html", html);
}

void handleSubmit() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    saveCredentials(ssid, password);
    server.send(200, "text/html", "Credentials Saved. Rebooting...");
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/html", "Invalid Request");
  }
}

void setup() {
  Serial.begin(115200);
  loadCredentials();
  if (ssid == "" || password == "") {
    startAPMode();
  } else {
    connectToWiFi();
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    server.handleClient();
  }


}
