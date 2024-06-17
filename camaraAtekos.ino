#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "esp_camera.h"
#include <HTTPClient.h>

// Incluir archivos HTML
#include "html/index_html.h"
#include "html/404_html.h"
#include "html/credentials_saved_html.h"

// Incluir archivos de configuración y pines de la cámara
#include "config.h"
#include "pines.h"

Preferences preferences;

const char* ssid_ap = "ESP32_AP";
const char* password_ap = "12345678";

WebServer server(80);
HTTPClient http;

String ssid;
String password;

const int timeCapture = 10000;
const unsigned long attemptInterval = 300000;  // 5 minutos
unsigned long lastAttemptTime = 0;

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

void setupPins() {
  pinMode(FLASH_LED_PIN, OUTPUT);
}

void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  Serial.println("Camera init succeeded");

  // Descartar los primeros fotogramas
  for (int i = 0; i < 2; i++) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) {
      esp_camera_fb_return(fb);
    }
    delay(100);
  }
}

camera_fb_t* capturePhoto() {
  digitalWrite(FLASH_LED_PIN, HIGH);  // Turn on flash LED
  delay(100);                         // Small delay to allow the LED to fully light up

  camera_fb_t* fb = esp_camera_fb_get();

  digitalWrite(FLASH_LED_PIN, LOW);  // Turn off flash LED

  if (!fb) {
    Serial.println("Camera capture failed");
  } else {
    Serial.println("Photo captured successfully");
  }

  return fb;
}

bool connectToServer(WiFiClientSecure& client, const char* server, uint16_t port) {
  client.setInsecure();  // No SSL certificate verification (use only for development)
  Serial.print("Connecting to server: ");
  Serial.print(server);
  Serial.print(":");
  Serial.println(port);

  if (!client.connect(server, port)) {
    Serial.println("Connection to server failed");
    return false;
  }
  Serial.println("Connected to server");
  return true;
}

void sendPhoto(WiFiClientSecure& client, camera_fb_t* fb) {
  if (!fb) {
    Serial.println("No photo to send");
    return;
  }

  String boundary = "----ESP32CAMBoundary";
  String head = "--" + boundary + "\r\n" + "Content-Disposition: form-data; name=\"photo\"; filename=\"esp32cam.jpg\"\r\n" + "Content-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  int contentLength = head.length() + fb->len + tail.length();

  client.println("POST " + String(ENDPOINT_API_UP_IMG) + " HTTP/1.1");
  client.println("Host: " + String(SERVER_URL));
  client.println("User-Agent: ESP32CAM/1.0");
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println("Content-Length: " + String(contentLength));
  client.println();
  client.print(head);

  size_t fb_len = fb->len;
  uint8_t* fb_buf = fb->buf;
  while (fb_len > 0) {
    size_t chunkSize = fb_len;
    if (chunkSize > 1024) {
      chunkSize = 1024;
    }
    client.write(fb_buf, chunkSize);
    fb_buf += chunkSize;
    fb_len -= chunkSize;
  }

  client.print(tail);

  int waitTime = 10000;  // Wait up to 10 seconds for a response
  long startTime = millis();
  while (client.connected() && (millis() - startTime < waitTime)) {
    if (client.available()) {
      String response = client.readString();
      Serial.println("Server response:");
      Serial.println(response);
      break;
    }
    delay(100);
  }

  client.stop();
  esp_camera_fb_return(fb);
  Serial.println("Photo sent and client disconnected");
}

void setup() {
  Serial.begin(115200);
  setupPins();
  loadCredentials();
  if (ssid == "" || password == "") {
    startAPMode();
  } else {
    connectToWiFi();
  }
}

void loop() {
  WiFiClientSecure client;

  if (WiFi.status() == WL_CONNECTED) {
    // Ejecutar funcionalidades adicionales cuando esté conectado al WiFi
    if (connectToServer(client, SERVER_URL, SERVER_PORT)) {
      camera_fb_t* fb = capturePhoto();
      sendPhoto(client, fb);
    } else {
      Serial.println("Retrying in 10 seconds...");
    }
    delay(timeCapture);  // Esperar 10 segundos antes de capturar y enviar otra foto
  } else {
    server.handleClient();  // Manejar las solicitudes del servidor web en modo AP
    if (millis() - lastAttemptTime > attemptInterval) {
      lastAttemptTime = millis();
      Serial.println("Retrying to connect to WiFi...");
      connectToWiFi();
    }
  }
}
