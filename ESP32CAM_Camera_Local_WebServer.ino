// --- 1. ต้องนิยามรุ่นบอร์ดก่อน Include เสมอ ---
#define CAMERA_MODEL_AI_THINKER 

#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "camera_pins.h"

// --- 2. ตั้งค่า WiFi ---
const char *ssid = "Worax_to_2.4G";
const char *password = "Toy.za1266";

WiFiUDP udp;
unsigned int localPort = 1234;

void startCameraServer(); 

void setup() {
  Serial.begin(115200); // ส่งข้อมูลไปหา STM32
  
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
  config.frame_size = FRAMESIZE_VGA;    // ชัดระดับ VGA (640x480)
  config.jpeg_quality = 10;
  config.fb_count = 2;

  // เริ่มต้นกล้อง
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) return;

  // เชื่อมต่อ WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  startCameraServer(); // เริ่มเว็บเซิร์ฟเวอร์ส่งภาพ
  udp.begin(localPort); // เริ่มรอรับคำสั่งจาก Python
  
  Serial.print("\nCamera Ready! IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // รับคำสั่ง UDP จาก Python
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buf[255];
    int len = udp.read(buf, 255);
    if (len > 0) buf[len] = 0;
    
    // ส่งต่อตัวอักษรที่ได้รับ (H, D, G, E) ไปให้ STM32 ผ่านสาย Serial
    Serial.print(buf); 
  }
}