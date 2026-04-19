#include <WiFi.h>
#include <WebServer.h>
#include <U8g2lib.h>
#include "control_page.h" // ไฟล์ที่เก็บหน้า index_html

// --- การตั้งค่าจอ OLED SH1106 (SDA=21, SCL=22) ---
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

const char* ssid = "Humi_Robot";
const char* password = "12345678";
WebServer server(80);

char current_emotion = 'N'; // เก็บสถานะอารมณ์ปัจจุบัน

// --- ฟังก์ชันวาดใบหน้าอารมณ์บนจอ OLED ---
void updateDisplay() {
  u8g2.clearBuffer();
  u8g2.drawCircle(64, 32, 28); // วาดกรอบหน้าวงกลมเป็นพื้นฐาน (ถ้าไม่ชอบให้คอมเมนต์ออกได้ครับ)

  switch (current_emotion) {
    case 'H': // ยิ้ม (Happy)
      u8g2.drawFilledEllipse(50, 25, 3, 5);   // ตาซ้าย
      u8g2.drawFilledEllipse(78, 25, 3, 5);   // ตาขวา
      u8g2.drawCircle(64, 35, 12, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT); // ปากยิ้ม
      break;

    case 'D': // เศร้า (saD) - ปรับปรุงตามที่เทสล่าสุด
      u8g2.drawLine(48, 20, 58, 16);          // คิ้วซ้าย (ยกขึ้นตรงกลาง)
      u8g2.drawLine(80, 20, 70, 16);          // คิ้วขวา (ยกขึ้นตรงกลาง)
      u8g2.drawLine(50, 26, 54, 28);          // ตาซ้ายเฉียงลง
      u8g2.drawLine(78, 26, 74, 28);          // ตาขวาเฉียงลง
      u8g2.drawCircle(64, 48, 10, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT); // ปากคว่ำ
      u8g2.drawLine(50, 32, 50, 40);          // น้ำตา
      break;

    case 'G': // โกรธ (anGry)
      u8g2.drawLine(42, 22, 58, 28);          // คิ้วซ้ายเฉียงลงหาจมูก
      u8g2.drawLine(86, 22, 70, 28);          // คิ้วขวาเฉียงลงหาจมูก
      u8g2.drawFilledEllipse(50, 30, 4, 4);   // ตาซ้าย
      u8g2.drawFilledEllipse(78, 30, 4, 4);   // ตาขวา
      u8g2.drawLine(54, 48, 74, 48);          // ปากเม้มตรง
      break;

    case 'E': // ตกใจ (Excited/Shock)
      u8g2.drawCircle(50, 25, 6);             // ตาโตซ้าย
      u8g2.drawCircle(78, 25, 6);             // ตาโตขวา
      u8g2.drawCircle(64, 45, 8);             // ปากอ้าวงกลม
      break;

    case 'Z': // หลับ (Sleepy)
      u8g2.drawLine(45, 28, 55, 28);          // หลับตาซ้าย
      u8g2.drawLine(73, 28, 83, 28);          // หลับตาขวา
      u8g2.setFont(u8g2_font_6x10_tf);        // เปลี่ยนฟอนต์เล็กน้อยสำหรับ Zzz
      u8g2.drawStr(90, 20, "Zzz"); 
      break;

    default: // หน้าปกติ (Normal)
      u8g2.drawFilledEllipse(50, 30, 4, 6); 
      u8g2.drawFilledEllipse(78, 30, 4, 6);
      u8g2.drawLine(54, 50, 74, 50);          // ปากตรงปกติ
      break;
  }
  u8g2.sendBuffer();
}

void setup() {
  // Serial สำหรับดูผ่านคอมพิวเตอร์ (Debug)
  Serial.begin(115200);
  
  // Serial2 สำหรับคุยกับ STM32 (16:RX, 17:TX)
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  
  // เริ่มต้นหน้าจอ
  u8g2.begin();
  updateDisplay();
  
  // ตั้งค่า WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("--- Humi Robot System Started ---");
  Serial.print("IP Address: "); Serial.println(WiFi.softAPIP());

  // เส้นทางสำหรับหน้าเว็บหลัก
  server.on("/", []() {
    server.send(200, "text/html", index_html);
    Serial.println("Client accessed Web Page");
  });

  // เส้นทางรับคำสั่งควบคุม (F, B, L, R, S, A, M, V0-V100)
  server.on("/control", []() {
    if (server.hasArg("cmd")) {
      String cmd = server.arg("cmd");
      
      // ส่งคำสั่งไปที่ STM32 ทันที
      Serial2.print(cmd); 
      
      // แสดงผลบน Serial Monitor เพื่อเช็คความถูกต้อง
      Serial.print("Web Command Sent -> STM32: ");
      Serial.println(cmd);
    }
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

void loop() {
  server.handleClient();

  // รับรหัสอารมณ์จาก STM32 เพื่อเปลี่ยนหน้าจอ
  if (Serial2.available()) {
    char incoming = (char)Serial2.read();
    
    // ตรวจสอบว่าเป็นรหัสอารมณ์ที่กำหนดไว้หรือไม่
    if (strchr("HGDENZ", incoming)) {
      current_emotion = incoming;
      Serial.print("Emotion Received: "); Serial.println(current_emotion);
      updateDisplay();
    }
  }
}