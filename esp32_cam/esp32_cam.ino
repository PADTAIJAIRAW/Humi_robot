#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// =========================
// WiFi
// =========================
const char* ssid = "Worax_to_2.4G";
const char* password = "Toy.za1266";

// =========================
// Flask YOLO Server
// เปลี่ยน IP ให้ตรงกับคอมคุณ
// =========================
const char* serverUrl = "http://192.168.1.108:5000/predict";

// =========================
// AI Thinker ESP32-CAM Pins
// =========================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_LED_PIN      4

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("ESP32-CAM IP: ");
  Serial.println(WiFi.localIP());
}

bool startCamera() {
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 6;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QQVGA;
    config.jpeg_quality = 8;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 2);
    s->set_contrast(s, 1);
    s->set_saturation(s, 0);
    s->set_gain_ctrl(s, 1);
    s->set_exposure_ctrl(s, 1);
    s->set_awb_gain(s, 1);
    s->set_wb_mode(s, 0);
    s->set_aec2(s, 1);
  }

  return true;
}

void sendFrameToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    return;
  }

  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(180);

  camera_fb_t *fb = esp_camera_fb_get();

  digitalWrite(FLASH_LED_PIN, LOW);

  if (!fb) {
    Serial.println("Capture fail");
    delay(1000);
    return;
  }

  Serial.print("Captured image size: ");
  Serial.println(fb->len);

  HTTPClient http;
  http.setTimeout(10000);
  http.begin(serverUrl);
  http.addHeader("Content-Type", "image/jpeg");

  int code = http.POST(fb->buf, fb->len);

  if (code > 0) {
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.print("HTTP error: ");
    Serial.println(code);
  }

  http.end();
  esp_camera_fb_return(fb);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  Serial.println("Starting ESP32-CAM...");

  connectWiFi();

  if (!startCamera()) {
    while (true) {
      delay(1000);
    }
  }

  Serial.println("Camera OK");
}

void loop() {
  sendFrameToServer();
  delay(3000);
}