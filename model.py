import cv2
import time
import socket
import csv
import os
from datetime import datetime
from ultralytics import YOLO

# ==========================================
# 1. ตั้งค่าการเชื่อมต่อ (โปรดเช็ค IP ให้ตรงกับกล้อง)
# ==========================================
ROBOT_IP = "192.168.1.104"  # IP ของ ESP32-CAM
UDP_PORT = 1234             # พอร์ตสำหรับส่งคำสั่ง
STREAM_URL = f"http://{ROBOT_IP}:81/stream"

# สร้าง Socket สำหรับส่ง UDP
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# ==========================================
# 2. ตั้งค่าไฟล์ CSV (จะบันทึกไว้ที่หน้า Desktop)
# ==========================================
desktop_path = os.path.join(os.path.expanduser('~'), 'Desktop')
LOG_FILE = os.path.join(desktop_path, 'humi_emotion_log.csv')

# สร้างหัวตารางถ้าไฟล์ยังไม่มี
if not os.path.exists(LOG_FILE):
    with open(LOG_FILE, mode='w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(['Timestamp', 'Emotion', 'Action', 'Command_Sent'])

def save_to_csv(emotion, cmd):
    """ฟังก์ชันบันทึกข้อมูลลงไฟล์ CSV"""
    now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    try:
        with open(LOG_FILE, mode='a', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow([now, emotion.upper(), 'STOP_10_SEC', cmd])
        print(f"📁 บันทึกข้อมูล: {emotion.upper()} ลง CSV เรียบร้อย")
    except Exception as e:
        print(f"❌ บันทึก CSV ไม่สำเร็จ: {e}")

# ==========================================
# 3. โหลดโมเดล YOLO และเปิดกล้อง
# ==========================================
print("⏳ กำลังโหลด AI Model...")
model = YOLO("best.pt")

print(f"🎥 กำลังเชื่อมต่อกล้อง: {STREAM_URL}")
cap = cv2.VideoCapture(STREAM_URL)
cap.set(cv2.CAP_PROP_BUFFERSIZE, 1) # ลดดีเลย์ภาพ

# ตัวแปรควบคุมเวลา
last_detect_time = 0
STOP_DURATION = 10  # หยุดรถ 10 วินาที
frame_count = 0

print("\n🚀 === ระบบ Humi AI พร้อมทำงาน (กด 'q' เพื่อเลิก) ===")

while True:
    ret, frame = cap.read()
    if not ret:
        print("❌ ไม่สามารถดึงภาพจากกล้องได้ (ลองเช็ค IP หรือปิด Browser)")
        time.sleep(1)
        continue

    frame_count += 1
    current_time = time.time()
    
    # ตรวจสอบว่ายังอยู่ในช่วงพัก 10 วินาทีหรือไม่
    is_paused = (current_time - last_detect_time) < STOP_DURATION

    # ประมวลผล AI ทุกๆ 2 เฟรมเพื่อความลื่นไหล
    if frame_count % 2 == 0:
        results = model(frame, verbose=False, conf=0.35) # ปรับความไวตรงนี้ (0.35)
        
        # ถ้าพ้นระยะ 10 วินาทีแล้ว และตรวจเจออะไรบางอย่าง
        if len(results) > 0 and len(results[0].boxes) > 0 and not is_paused:
            box = results[0].boxes[0]
            label = model.names[int(box.cls[0])].lower()
            
            # แปลง Label เป็นรหัสสั่งงาน
            cmd = ''
            if label == "happy": cmd = 'H'
            elif label == "sad": cmd = 'D'
            elif label == "angry": cmd = 'G'
            
            if cmd:
                print(f"✅ ตรวจพบ: {label.upper()}")
                try:
                    # ส่งอารมณ์ไปโชว์ที่หน้าจอก่อน (H, D, G)
                    sock.sendto(cmd.encode(), (ROBOT_IP, UDP_PORT)) 
                    
                    # เว้นจังหวะนิดเดียวเพื่อให้ Serial ส่งข้อมูลเสร็จ
                    time.sleep(0.05) 
                    
                    # ส่งคำสั่งหยุดรถตามไป
                    sock.sendto(b'E', (ROBOT_IP, UDP_PORT)) 
                except:
                    print("❌ ส่งข้อมูลล้มเหลว")
                
                # 2. บันทึกลงไฟล์ CSV
                save_to_csv(label, cmd)
                
                # 3. เริ่มจับเวลาพักเครื่อง
                last_detect_time = current_time
                is_paused = True

        # วาดกรอบบนจอ
        display_frame = results[0].plot() if results else frame
    else:
        display_frame = frame

    # --- ส่วนการแสดงผลหน้าจอ (UI) ---
    if is_paused:
        time_left = int(STOP_DURATION - (current_time - last_detect_time))
        # แสดงแถบสีส้มด้านบนแจ้งเตือนการหยุดรถ
        cv2.rectangle(display_frame, (0, 0), (640, 50), (0, 165, 255), -1)
        cv2.putText(display_frame, f"ROBOT STOPPED: WAIT {time_left}s", (150, 35), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)

    # โชว์ภาพ
    cv2.imshow("Humi Wireless AI Vision", display_frame)

    # กด q เพื่อปิดโปรแกรม
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# คืนทรัพยากร
cap.release()
sock.close()
cv2.destroyAllWindows()
print("\n🏁 ปิดโปรแกรมเรียบร้อย")