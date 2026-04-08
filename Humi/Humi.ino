#include <WiFi.h>
#include <WebServer.h>
#include "control_page.h" // ไฟล์ที่เก็บหน้า index_html

const char* ssid = "Humi_Robot";
const char* password = "12345678";

WebServer server(80);

void setup() {
  Serial.begin(115200);
  
  // สื่อสารกับ STM32: ขา 16(RX), 17(TX) ความเร็ว 115200
  Serial2.begin(115200, SERIAL_8N1, 16, 17); 
  
  // ตั้งค่า Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // หน้าหลัก
  server.on("/", []() { 
    // ตรวจสอบว่าใน control_page.h ใช้ชื่อตัวแปรว่า index_html หรือ MAIN_page 
    // ถ้าใช้ตามโค้ด UI ที่ผมส่งให้ก่อนหน้า ให้เปลี่ยนเป็น index_html ครับ
    server.send(200, "text/html", index_html); 
  });
  
  // --- ส่วนรับคำสั่งควบคุมแบบรวมศูนย์ (ตามที่ UI ส่งมา) ---
  server.on("/control", []() {
    if (server.hasArg("cmd")) {
      String cmd = server.arg("cmd");
      
      // ส่งคำสั่งออกไปหา STM32
      Serial2.print(cmd); 
      
      // Debug ดูใน Serial Monitor ของ ESP32
      Serial.print("Sent to STM32: ");
      Serial.println(cmd);
    }
    server.send(200, "text/plain", "OK");
  });

  server.begin();
  Serial.println("HTTP Server Started");
}

void loop() { 
  server.handleClient(); 
}