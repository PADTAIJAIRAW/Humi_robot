#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

HardwareSerial Serial2(PA3, PA2);   // รับคำสั่งจาก ESP32
HardwareSerial Serial3(PB11, PB10); // รับหน้าอารมณ์จาก AI
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// พินมอเตอร์
#define L_IN1 PA0
#define L_IN2 PA1
#define R_IN1 PA8
#define R_IN2 PB13

// พินเซนเซอร์เดินตามเส้น
const int S[] = {PB0, PB1, PB5, PB4, PB3}; 

// ตัวแปรควบคุม
char current_cmd = 'S';     
char current_emotion = 'N'; 
bool is_auto = false;

// ตัวแปรควบคุมตาพริบ
unsigned long last_blink_time = 0;
int blink_interval = 3000;
bool is_blinking = false;

void setup() {
  Serial2.begin(115200); 
  Serial3.begin(115200); 
  
  u8g2.begin();
  pinMode(L_IN1, OUTPUT); pinMode(L_IN2, OUTPUT);
  pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  for(int i=0; i<5; i++) pinMode(S[i], INPUT);

  // โชว์หน้าเริ่มต้น
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(20, 40, "GO HUMI!"); 
  u8g2.sendBuffer();
  
  // สั่งหยุดมอเตอร์ชัวร์ๆ ตอนเปิดเครื่อง
  digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, LOW);
  digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
}

void updateDisplay() {
  u8g2.clearBuffer();
  if (current_emotion == 'H') { 
    u8g2.drawCircle(40, 25, 10); u8g2.drawCircle(88, 25, 10);
    u8g2.drawRFrame(44, 45, 40, 10, 5); 
  } else if (current_emotion == 'S') { 
    u8g2.drawCircle(40, 35, 5); u8g2.drawCircle(88, 35, 5);
    u8g2.drawArc(64, 60, 15, 0, 4); 
  } else if (current_emotion == 'E') { 
    u8g2.drawLine(30, 20, 50, 30); u8g2.drawLine(78, 30, 98, 20);
    u8g2.drawDisc(40, 35, 7); u8g2.drawDisc(88, 35, 7);
  } else if (current_emotion == 'W') { // Wink Eyes
    u8g2.drawDisc(40, 30, 8);          
    u8g2.drawLine(80, 30, 96, 30);     
    u8g2.drawRFrame(54, 45, 20, 8, 3); 
  } else { 
    if (is_blinking) {
      u8g2.drawLine(32, 30, 48, 30); u8g2.drawLine(80, 30, 96, 30);
    } else {
      u8g2.drawDisc(40, 30, 8); u8g2.drawDisc(88, 30, 8);
    }
  }
  u8g2.sendBuffer();
}

// ควบคุมมือ (Digital ล้วน ทรงพลังและไม่มีวันค้าง)
void runManualControl() {
  switch (current_cmd) {
    case 'F': 
      digitalWrite(L_IN1, HIGH); digitalWrite(L_IN2, LOW);
      digitalWrite(R_IN1, HIGH); digitalWrite(R_IN2, LOW);
      break;
    case 'B': 
      digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, HIGH);
      digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, HIGH);
      break;
    case 'R': 
      digitalWrite(L_IN1, HIGH); digitalWrite(L_IN2, LOW);
      digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, HIGH);
      break;
    case 'L': 
      digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, HIGH);
      digitalWrite(R_IN1, HIGH); digitalWrite(R_IN2, LOW);
      break;
    case 'S': 
    default: 
      digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, LOW);
      digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
      break;
  }
}

// เดินตามเส้น (Digital ล้วน แก้ปัญหาล็อก PWM)
void runLineFollower() {
  int s0=digitalRead(S[0]), s1=digitalRead(S[1]), s2=digitalRead(S[2]), s3=digitalRead(S[3]), s4=digitalRead(S[4]);
  
  // ลอจิก: 1 = เจอเส้นดำ (ไฟดับ), 0 = เจอพื้นขาว (ไฟติด)
  if (s2 == 1) { 
    // ตัวกลางทับเส้นดำ -> พุ่งไปข้างหน้า
    digitalWrite(L_IN1, HIGH); digitalWrite(L_IN2, LOW);
    digitalWrite(R_IN1, HIGH); digitalWrite(R_IN2, LOW);
  } 
  else if (s1 == 1 || s0 == 1) { 
    // ตัวซ้ายทับเส้นดำ -> เลี้ยวซ้ายกลับเข้าเส้น
    digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, HIGH);
    digitalWrite(R_IN1, HIGH); digitalWrite(R_IN2, LOW);
  } 
  else if (s3 == 1 || s4 == 1) { 
    // ตัวขวาทับเส้นดำ -> เลี้ยวขวากลับเข้าเส้น
    digitalWrite(L_IN1, HIGH); digitalWrite(L_IN2, LOW);
    digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, HIGH);
  } 
  else { 
    // หลุดเส้น -> เบรก
    digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, LOW);
    digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
  }
}

void loop() {
  // --- รับคำสั่งจาก ESP32 ---
  if (Serial2.available() > 0) {
    char inChar = (char)Serial2.read();
    
    if (inChar == 'F' || inChar == 'B' || inChar == 'L' || inChar == 'R' || inChar == 'S') {
      current_cmd = inChar;
      is_auto = false;
    } 
    else if (inChar == 'M') {
      is_auto = false; 
      current_cmd = 'S'; 
      runManualControl(); // สลับโหมดปุ๊บ เบรกปั๊บ
    } 
    else if (inChar == 'A') {
      is_auto = true;
    } 
    else if (inChar == 'V') {
      // ดึงตัวเลขความเร็วทิ้งไปเลย เพื่อกันขยะตกค้างในระบบ (เพราะเราใช้ HIGH 100% แล้ว)
      Serial2.parseInt(); 
    }
  }

  // --- รับหน้าอารมณ์ ---
  if (Serial3.available()) {
    char emoChar = (char)Serial3.read();
    if (emoChar == 'H' || emoChar == 'S' || emoChar == 'E' || emoChar == 'N' || emoChar == 'W') {
      if (emoChar != current_emotion) {
        current_emotion = emoChar;
        updateDisplay();
      }
    }
  }

  // --- ลอจิกกระพริบตา ---
  if (current_emotion == 'N') {
    unsigned long current_time = millis();
    if (!is_blinking && (current_time - last_blink_time > blink_interval)) {
      is_blinking = true; last_blink_time = current_time; updateDisplay();
    }
    if (is_blinking && (current_time - last_blink_time > 150)) {
      is_blinking = false; last_blink_time = current_time;
      blink_interval = random(2000, 5000); updateDisplay();
    }
  }

  // --- เรียกลอจิกมอเตอร์ ---
  if (is_auto) runLineFollower(); 
  else runManualControl();
}