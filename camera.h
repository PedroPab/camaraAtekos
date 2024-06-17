#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"
#include <HTTPClient.h>

// Incluir archivos de configuración y pines de la cámara
#include "config.h"
#include "pines.h"

extern HTTPClient http;

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

  // Descartar los primeros fotogramas para evitar fotos verdes
  for (int i = 0; i < 10; i++) {
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

void sendPhoto(camera_fb_t* fb) {
  if (!fb) {
    Serial.println("No photo to send");
    return;
  }

  String boundary = "----ESP32CAMBoundary";
  String head = "--" + boundary + "\r\n" + "Content-Disposition: form-data; name=\"photo\"; filename=\"esp32cam.jpg\"\r\n" + "Content-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  int contentLength = head.length() + fb->len + tail.length();

  // Crear un buffer para el cuerpo del mensaje
  uint8_t* body = (uint8_t*)malloc(contentLength);
  if (!body) {
    Serial.println("Failed to allocate memory for body");
    esp_camera_fb_return(fb);
    return;
  }

  // Copiar el encabezado, la imagen y el pie al buffer
  memcpy(body, head.c_str(), head.length());
  memcpy(body + head.length(), fb->buf, fb->len);
  memcpy(body + head.length() + fb->len, tail.c_str(), tail.length());

  String serverName = String(SERVER_URL) + ":" + String(SERVER_PORT) + String(ENDPOINT_API_UP_IMG);
  http.begin(serverName);  // URL del servidor
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
  
  int httpResponseCode = http.POST(body, contentLength);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Server response:");
    Serial.println(response);
  } else {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
    Serial.println("Error message: " + http.errorToString(httpResponseCode));
    if (httpResponseCode < 0) {
      String response = http.getString();
      Serial.println("Server response:");
      Serial.println(response);
    }
  }

  http.end();
  esp_camera_fb_return(fb);
  free(body); // Liberar la memoria asignada
  Serial.println("Photo sent and client disconnected");
}

#endif // CAMERA_H
