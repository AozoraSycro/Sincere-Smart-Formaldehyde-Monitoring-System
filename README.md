# 🌬️ Sincere Sensor — ระบบตรวจวัดคุณภาพอากาศแบบ Offline

ระบบตรวจวัดความเข้มข้นของก๊าซด้วย ESP32 พร้อม Dashboard แบบ Real-time ผ่าน Wi-Fi ในตัว ไม่ต้องพึ่งอินเทอร์เน็ต ทำงานได้ทันทีโดยเชื่อมต่อตรงกับ ESP32

---

## 📋 ภาพรวมโปรเจกต์

ESP32 ทำหน้าที่เป็นทั้ง Access Point และ Web Server ในตัวเดียว โดยอ่านค่าจากเซนเซอร์ก๊าซผ่าน UART แล้วแสดงผลบน Dashboard ผ่านเบราว์เซอร์บนมือถือหรือคอมพิวเตอร์ที่เชื่อมต่อ Wi-Fi เดียวกัน พร้อมแจ้งเตือนด้วย LED 10 ดวงและ Buzzer

```
Sensor ZE08-CH2O  ──UART──►  ESP32  ──Wi-Fi──►  มือถือ / คอมฯ
                           │
                    LED x10 + Buzzer
```

---

## ✨ ฟีเจอร์

- **Real-time Dashboard** — แสดงค่า PPM อัปเดตทุก 1 วินาที
- **กราฟย้อนหลัง** — แสดงประวัติข้อมูล 30 วินาทีล่าสุดด้วย Canvas
- **3 ระดับแจ้งเตือน** — SAFE / WARNING / DANGER พร้อมสีสัญญาณ
- **LED Bar Graph** — LED 10 ดวงแสดงระดับความเข้มข้นแบบ Visual
- **Buzzer** — ส่งเสียงเตือนอัตโนมัติเมื่อค่าเกินขีดอันตราย
- **Offline 100%** — ไม่ต้องต่ออินเทอร์เน็ต ไม่ต้องโหลด Library ภายนอก
- **รองรับมือถือ** — UI Responsive ปรับตามขนาดหน้าจออัตโนมัติ

---

## 🔧 ฮาร์ดแวร์ที่ใช้

| อุปกรณ์ | รายละเอียด |
|---|---|
| ESP32 | Development Board (ทุกรุ่นที่มี UART2) |
| เซนเซอร์ก๊าซ | Sensor ZE08-CH2O |
| LED | 10 ดวง (พร้อม Resistor 220Ω ต่อดวง) |
| Buzzer | Active Buzzer 5V |

---

## 🔌 การต่อวงจร

```
เซนเซอร์            ESP32
─────────────────────────────
Pin 5 (RXD)  ──►  GPIO 27 (TX)
Pin 6 (TXD)  ──►  GPIO 26 (RX)
VCC          ──►  3.3V หรือ 5V (ตามสเปก)
GND          ──►  GND

LED (ดวงที่ 1-10)  ──►  GPIO 13, 14, 32, 33, 16, 17, 18, 19, 21, 22
Buzzer             ──►  GPIO 25
```

> ⚠️ ต่อ Resistor 220Ω อนุกรมกับ LED ทุกดวงเพื่อป้องกันกระแสเกิน

---

## 💻 การติดตั้งและใช้งาน

### 1. เตรียม Arduino IDE

ติดตั้ง ESP32 Board Package โดยเพิ่ม URL นี้ใน **File → Preferences → Additional Board Manager URLs**:

```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

จากนั้นไปที่ **Tools → Board → Board Manager** แล้วค้นหาและติดตั้ง `esp32`

### 2. เลือก Library ที่ต้องใช้

Library ทั้งหมดเป็น Built-in ของ ESP32 ไม่ต้องติดตั้งเพิ่ม:
- `WiFi.h`
- `WebServer.h`
- `HardwareSerial.h`

### 3. อัปโหลดโค้ด

1. เปิดไฟล์ `sincere_sensor.ino` ใน Arduino IDE
2. เลือก Board: **ESP32 Dev Module** (เปลื่ยนตามที่ใช้)
3. เลือก Port ที่ถูกต้อง
4. กด **Upload**

### 4. เชื่อมต่อและใช้งาน

1. เปิดเครื่อง ESP32
2. เชื่อมต่อ Wi-Fi บนมือถือหรือคอมพิวเตอร์:
   **ชื่อ Wi-Fi และ Password สามารถเปลื่ยนได้**
   - **SSID:** `Sincere_Sensor`
   - **Password:** `12345678`
4. เปิดเบราว์เซอร์แล้วไปที่ `http://192.168.4.1`

---

## 📊 เกณฑ์การแจ้งเตือน

| ระดับ | ค่า PPM | สี | LED |
|---|---|---|---|
| 🟢 SAFE | < 0.05 | เขียว | 0–4 ดวง |
| 🟡 WARNING | 0.05 – 0.079 | เหลือง | 5–7 ดวง |
| 🔴 DANGER | ≥ 0.08 | แดง | 8–10 ดวง |

---

## 📡 API Endpoints

| Endpoint | Method | Response | คำอธิบาย |
|---|---|---|---|
| `/` | GET | `text/html` | หน้า Dashboard |
| `/api/data` | GET | `application/json` | ค่า PPM ปัจจุบัน |

ตัวอย่าง Response จาก `/api/data`:
```json
{"ppm":0.045}
```

---

## 📁 โครงสร้างไฟล์

```
sincere-sensor/
├── sincere_sensor.ino   # โค้ดหลัก ESP32
└── README.md            # ไฟล์นี้
```

---

## ⚙️ การปรับแต่ง

แก้ไขค่าตัวแปรต่อไปนี้ในโค้ดตามต้องการ:

```cpp
// เปลี่ยนชื่อและรหัสผ่าน Wi-Fi
const char* ap_ssid = "Sincere_Sensor";
const char* ap_pass = "12345678";

// เปลี่ยนขีดจำกัดการแจ้งเตือน (หน่วย PPM)
// WARNING threshold: แก้ใน JavaScript ที่ ppm >= 0.05
// DANGER threshold:  แก้ในทั้ง JavaScript และ loop() ที่ currentPPM >= 0.08
```

---

## 🛠️ Troubleshooting

**เซนเซอร์ไม่มีค่า (แสดง 0.000 ตลอด)**
- ตรวจสอบการต่อสาย RX/TX ว่าสลับกันถูกต้อง (TX เซนเซอร์ → RX ESP32)
- ตรวจสอบไฟเลี้ยงเซนเซอร์ว่าตรงตามสเปก
- เปิด Serial Monitor (115200 baud) ดู log การทำงาน

**เชื่อมต่อ Wi-Fi ได้แต่เปิดเว็บไม่ได้**
- ตรวจสอบว่าพิมพ์ `http://192.168.4.1` (ไม่ใช่ https)
- บางครั้งมือถือจะถามว่า "เครือข่ายนี้ไม่มีอินเทอร์เน็ต ใช้งานต่อไปหรือไม่" ให้กด **ใช้งานต่อไป**

**LED หรือ Buzzer ไม่ทำงาน**
- ตรวจสอบหมายเลข GPIO ว่าตรงกับโค้ด
- ตรวจสอบว่า Buzzer เป็นชนิด Active (ไม่ใช่ Passive)

---

## 📜 License

MIT License — ใช้งานและดัดแปลงได้อย่างอิสระ

---
