#include <Arduino.h>

HardwareSerial Serial2(PA3, PA2);   
HardwareSerial Serial3(PB11, PB10); 

#define L_IN1 PA0  
#define L_IN2 PA1  
#define R_IN1 PA8  
#define R_IN2 PB0  
#define LED_S PA5

const int S[] = {PB9, PB8, PB7, PB6, PB5}; 

int speedPercent = 50; 
int baseSpeed = 160;   
char current_cmd = 'S';     
bool is_auto = false;
bool is_ai_stop = false;
unsigned long stop_timer = 0;

void setMotor(int L_input, int R_input) {
  int minPWM = 130; 
  int R = L_input; 
  int L = R_input;

  if (L > 0) L = map(L, 0, 255, minPWM, 255);
  else if (L < 0) L = map(L, -255, 0, -255, -minPWM);
  
  if (R > 0) R = map(R, 0, 255, minPWM, 255);
  else if (R < 0) R = map(R, -255, 0, -255, -minPWM);

  L = constrain(L, -255, 255);
  R = constrain(R, -255, 255);

  if (L >= 0) { analogWrite(L_IN1, 0); analogWrite(L_IN2, abs(L)); } 
  else { analogWrite(L_IN1, abs(L)); analogWrite(L_IN2, 0); }

  if (R >= 0) { analogWrite(R_IN1, 0); analogWrite(R_IN2, abs(R)); } 
  else { analogWrite(R_IN1, abs(R)); analogWrite(R_IN2, 0); }
}

void setup() {
  Serial2.begin(115200); 
  Serial3.begin(115200); 
  pinMode(L_IN1, OUTPUT); pinMode(L_IN2, OUTPUT);
  pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  for(int i=0; i<5; i++) pinMode(S[i], INPUT_PULLUP);
  setMotor(0, 0);
}

void runManualControl() {
  switch (current_cmd) {
    case 'R': setMotor(baseSpeed, baseSpeed); break;
    case 'L': setMotor(-baseSpeed, -baseSpeed); break;
    case 'F': setMotor(-baseSpeed, baseSpeed); break; 
    case 'B': setMotor(baseSpeed, -baseSpeed); break; 
    case 'S': default: setMotor(0, 0); break;
  }
}

void runLineFollower() {
  int s2 = digitalRead(S[2]); 
  int s1 = digitalRead(S[1]); 
  int s3 = digitalRead(S[3]); 
  if (s2 == 0) { setMotor(-baseSpeed, baseSpeed); } 
  else if (s1 == 0) { setMotor(-baseSpeed, -baseSpeed); } 
  else if (s3 == 0) { setMotor(baseSpeed, baseSpeed); } 
  else { setMotor(0, 0); }
}

void loop() {
  // 1. รับคำสั่งจากหน้าเว็บ/ESP32 จอ (Serial2)
  if (Serial2.available() > 0) {
    char inChar = (char)Serial2.read();
    if (inChar == 'V') {
      int val = Serial2.parseInt();
      if (val >= 0 && val <= 100) {
        speedPercent = val;
        baseSpeed = map(speedPercent, 0, 100, 0, 255);
      }
    }
    // เมื่อกด F, B, L, R, S ให้หยุดโหมด Auto ทันทีเพื่อให้ Manual ทำงานได้
    else if (strchr("FBLRS", inChar)) { 
      current_cmd = inChar; 
      is_auto = false; 
      is_ai_stop = false; // ถ้ากดมือเอง ให้ยกเลิกการหยุดของ AI ด้วย
    }
    else if (inChar == 'M') { is_auto = false; current_cmd = 'S'; setMotor(0,0); Serial2.print('N'); }
    else if (inChar == 'A') { is_auto = true; is_ai_stop = false; }
  }

  // 2. รับคำสั่งจาก AI (Serial3)
  while (Serial3.available() > 0) {
    char camCmd = Serial3.read();
    if (camCmd == 'E') {
      is_ai_stop = true;
      stop_timer = millis() + 10000;
      setMotor(0, 0);
      //Serial2.print('E');
    } 
    else if (camCmd == 'H' || camCmd == 'D' || camCmd == 'G') {
      Serial2.print(camCmd);
    }
  }

  // 3. ส่วนตัดสินใจขับเคลื่อน (สำคัญ: แยกออกมาข้างนอกห้ามมี return ขวาง)
  if (is_ai_stop) {
    // ถ้า AI สั่งหยุด ให้หยุดมอเตอร์และเช็คเวลา
    setMotor(0, 0);
    if (millis() > stop_timer) { 
      is_ai_stop = false; 
      Serial2.print('N'); 
    }
  } 
  else {
    // ถ้า AI ไม่ได้สั่งหยุด ให้ทำงานตามโหมดที่เลือกไว้
    if (is_auto) {
      runLineFollower(); 
    } else {
      runManualControl();
    }
  }
}