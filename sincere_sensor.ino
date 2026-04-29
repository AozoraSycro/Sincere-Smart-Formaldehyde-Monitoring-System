// ============================================================
// โปรเจกต์: Sincere Sensor — ระบบตรวจวัดคุณภาพอากาศแบบ Offline
// ฮาร์ดแวร์: ESP32 + Sensor ZE08-CH2O (UART) + LED x10 + Buzzer
// การทำงาน: ESP32 ปล่อย Wi-Fi (Access Point) ให้มือถือเชื่อมต่อ
//           แล้วเปิดเว็บ Dashboard ดูค่าแบบ Real-time ได้เลย
// ============================================================

#include <WiFi.h>        // ไลบรารีสำหรับควบคุม Wi-Fi บน ESP32
#include <WebServer.h>   // ไลบรารีสำหรับสร้าง HTTP Web Server
#include <HardwareSerial.h> // ไลบรารีสำหรับ UART (รับส่งข้อมูลอนุกรม)

// ============================================================
// ตั้งค่าชื่อและรหัสผ่าน Wi-Fi ที่ ESP32 จะปล่อยออกมา (Access Point)
// มือถือหรือคอมพิวเตอร์ต้องเชื่อมต่อ Wi-Fi นี้ก่อน แล้วค่อยเปิดเบราว์เซอร์
// ============================================================
// ชื่อ Wi-Fi และ Password สามารถเปลื่ยนได้
const char* ap_ssid = "Sincere_Sensor"; // ชื่อ Wi-Fi ที่จะแสดงในรายการ 
const char* ap_pass = "12345678";       // รหัสผ่าน (ต้องมีอย่างน้อย 8 ตัวอักษร)

// ============================================================
// กำหนดขา (PIN) ที่ใช้เชื่อมต่อกับอุปกรณ์ต่างๆ
// ============================================================
#define RX_PIN 26     // ขารับข้อมูล (RX) ต่อเข้ากับขา TXD ของเซนเซอร์
#define TX_PIN 27     // ขาส่งข้อมูล (TX) ต่อเข้ากับขา RXD ของเซนเซอร์
#define BUZZER_PIN 25 // ขาควบคุมกระดิ่งเตือน

// อาร์เรย์เก็บหมายเลขขาของ LED ทั้ง 10 ดวง (ใช้แสดงระดับความอันตราย)
// LED ดวงที่ 1 = ledPins[0] = ขา 13, ..., LED ดวงที่ 10 = ledPins[9] = ขา 22
const int ledPins[10] = {13, 14, 32, 33, 16, 17, 18, 19, 21, 22};

// ============================================================
// สร้าง Web Server ที่รองรับ HTTP บนพอร์ต 80 (พอร์ตมาตรฐาน)
// ทำให้เบราว์เซอร์สามารถเข้าถึงได้ด้วย http://192.168.4.1
// ============================================================
WebServer server(80);

// ตัวแปรสำหรับเก็บค่าความเข้มข้นของก๊าซ (PPM) ล่าสุดที่อ่านได้
// ประกาศเป็น float เพื่อให้แสดงทศนิยมได้
float currentPPM = 0.0;

// ============================================================
// HTML ของหน้า Dashboard ที่เก็บไว้ใน Flash Memory (PROGMEM)
// เหตุผลที่ใช้ PROGMEM: ประหยัด RAM ของ ESP32 ซึ่งมีจำกัด (~320KB)
// เป็น Offline Dashboard 100% — ไม่ต้องพึ่งอินเทอร์เน็ต ไม่ต้องโหลด library ภายนอก
// ============================================================
const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <!-- viewport: ปรับขนาดให้พอดีหน้าจอมือถือ -->
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sincere Local Dashboard</title>
    <style>
        /* ธีมมืด พื้นหลังสีดำ ตัวอักษรสีขาว */
        body { font-family: sans-serif; background: #121212; color: white; text-align: center; margin: 0; padding: 20px; }
        
        /* การ์ดแสดงข้อมูล: มีขอบสีฟ้าด้านซ้าย เพื่อความสวยงาม */
        .card { background: #1e1e1e; padding: 20px; border-radius: 15px; margin-bottom: 20px; border-left: 5px solid #3498db; }
        
        /* ตัวเลขค่า PPM ขนาดใหญ่ สีฟ้า */
        .val { font-size: 60px; font-weight: bold; color: #3498db; }
        
        /* ข้อความสถานะ */
        .status { font-size: 24px; margin-top: 10px; }
        
        /* กล่องสำหรับ Canvas กราฟ */
        .chart-container { position: relative; height: 250px; width: 100%; background: #2a2a2a; border-radius: 10px; overflow: hidden; }
        canvas { display: block; width: 100%; height: 100%; }
        
        /* Class สำหรับโหมดอันตราย: เปลี่ยนสีกรอบและตัวเลขเป็นสีแดง */
        .danger { color: #e74c3c !important; border-color: #e74c3c !important; }
    </style>
</head>
<body>
    <h1>Sincere <span style="color:#3498db">Local Monitor</span></h1>
    
    <!-- การ์ดหลัก: แสดงสถานะและค่า PPM ปัจจุบัน -->
    <div class="card" id="mainCard">
        <div id="statusText" class="status">คุณภาพอากาศ: ปกติ</div>
        <div class="val"><span id="ppmDisplay">0.000</span> <span style="font-size:20px">PPM</span></div>
    </div>
    
    <!-- การ์ดกราฟ: แสดงประวัติค่า PPM ย้อนหลัง 30 จุด -->
    <div class="card">
        <div class="chart-container">
            <canvas id="myCanvas"></canvas>
        </div>
    </div>

    <script>
        const MAX_POINTS = 30; // จำนวนจุดสูงสุดที่แสดงบนกราฟ (เก็บข้อมูล 30 วินาทีย้อนหลัง)
        let ppmData = [];      // อาร์เรย์เก็บค่า PPM สำหรับวาดกราฟ
        
        // ============================================================
        // ฟังก์ชัน drawChart(): วาดกราฟเส้นด้วย Canvas API
        // เรียกใช้ทุกครั้งที่มีข้อมูลใหม่หรือมีการหมุนหน้าจอ
        // ============================================================
        function drawChart() {
            const canvas = document.getElementById('myCanvas');
            const ctx = canvas.getContext('2d'); // ขอ context สำหรับวาด 2D
            
            // ปรับ resolution ให้ตรงกับขนาดจริงของ element (สำคัญมากสำหรับมือถือ)
            canvas.width = canvas.offsetWidth;
            canvas.height = canvas.offsetHeight;
            const w = canvas.width;
            const h = canvas.height;

            ctx.clearRect(0, 0, w, h); // ล้างกราฟเก่าออกก่อนวาดใหม่
            if(ppmData.length < 2) return; // ต้องมีข้อมูลอย่างน้อย 2 จุดจึงจะวาดเส้นได้

            // คำนวณค่าสูงสุดเพื่อกำหนดเพดานสเกลของแกน Y
            const minPPM = 0;
            const maxVal = Math.max(...ppmData, 0.1); // ใช้ spread operator หาค่าสูงสุด
            const maxPPM = maxVal * 1.2; // บวกพื้นที่เพิ่มอีก 20% ไม่ให้กราฟชิดขอบบน
            
            // ระยะห่างแนวนอนระหว่างแต่ละจุดข้อมูล
            const dx = w / (MAX_POINTS - 1);

            // ---- วาดเส้นประสีแดง = เส้นขีดจำกัดอันตราย (0.08 PPM) ----
            // คำนวณตำแหน่ง Y จากค่า 0.08 ในสเกลของกราฟ
            const limitY = h - ((0.08 - minPPM) / (maxPPM - minPPM) * h);
            ctx.beginPath();
            ctx.strokeStyle = 'rgba(231, 76, 60, 0.8)'; // สีแดงโปร่งแสง
            ctx.setLineDash([5, 5]); // เส้นประ (5px วาด, 5px เว้น)
            ctx.lineWidth = 2;
            ctx.moveTo(0, limitY);
            ctx.lineTo(w, limitY);
            ctx.stroke();
            ctx.setLineDash([]); // รีเซ็ตกลับเป็นเส้นทึบสำหรับกราฟหลัก

            // ---- วาดเส้นกราฟข้อมูลสีฟ้า ----
            ctx.beginPath();
            ctx.strokeStyle = '#3498db';
            ctx.lineWidth = 4;
            ctx.lineJoin = 'round'; // มุมโค้งมน ดูนุ่มนวลกว่า miter
            for(let i=0; i<ppmData.length; i++) {
                const x = i * dx; // ตำแหน่ง X ของจุดที่ i
                // แปลงค่า PPM → ตำแหน่ง Y (กลับทิศเพราะ Canvas นับ Y จากบน)
                const y = h - ((ppmData[i] - minPPM) / (maxPPM - minPPM) * h);
                if(i===0) ctx.moveTo(x, y); // จุดแรก: ยก Pen ไปวาง
                else ctx.lineTo(x, y);       // จุดถัดไป: ลากเส้น
            }
            ctx.stroke(); // วาดเส้นออกมาจริงๆ
        }

        // ============================================================
        // ฟังก์ชัน updateData(): ดึงข้อมูลจาก ESP32 แล้วอัปเดต UI
        // ใช้ async/await + fetch API เพื่อเรียก HTTP แบบไม่บล็อก
        // ============================================================
        async function updateData() {
            try {
                // เรียก API ที่ ESP32 → ได้ JSON กลับมา เช่น {"ppm":0.045}
                const res = await fetch('/api/data');
                const data = await res.json();
                const ppm = data.ppm;
                
                // อัปเดตตัวเลขบนหน้าจอ ทศนิยม 3 ตำแหน่ง
                document.getElementById('ppmDisplay').innerText = ppm.toFixed(3);
                
                // เพิ่มค่าใหม่เข้าอาร์เรย์ แล้วลบค่าเก่าออกถ้าเกิน MAX_POINTS
                ppmData.push(ppm);
                if (ppmData.length > MAX_POINTS) ppmData.shift(); // shift() = ลบตัวแรก
                drawChart(); // วาดกราฟใหม่ทันที
                
                // ---- ตัดสินใจแสดงสถานะตาม 3 ระดับ ----
                const card = document.getElementById('mainCard');
                const st = document.getElementById('statusText');
                
                if (ppm >= 0.08) {
                    // ระดับอันตราย: เปลี่ยนการ์ดเป็นสีแดงทั้งหมด
                    card.classList.add('danger');
                    st.innerText = "สถานะ: อันตราย! (DANGER)";
                    st.style.color = "#e74c3c";
                } else if (ppm >= 0.05) {
                    // ระดับเฝ้าระวัง: ขอบการ์ดเปลี่ยนเป็นสีเหลือง
                    card.classList.remove('danger');
                    card.style.borderColor = "#f1c40f";
                    st.innerText = "สถานะ: เฝ้าระวัง (WARNING)";
                    st.style.color = "#f1c40f";
                } else {
                    // ระดับปกติ: ขอบการ์ดสีฟ้า ตัวอักษรสีเขียว
                    card.classList.remove('danger');
                    card.style.borderColor = "#3498db";
                    st.innerText = "สถานะ: ปกติ (SAFE)";
                    st.style.color = "#2ecc71";
                }
            } catch (e) {
                // หากเชื่อมต่อหลุด (เช่น ออกนอกระยะ Wi-Fi) แค่ log ไว้ ไม่ crash
                console.log("Disconnected");
            }
        }

        // เมื่อผู้ใช้หมุนหน้าจอมือถือ Canvas จะ resize → วาดกราฟใหม่
        window.addEventListener('resize', drawChart);

        // เรียก updateData() ซ้ำทุก 1000ms (1 วินาที) เพื่อ Real-time update
        setInterval(updateData, 1000);
    </script>
</body>
</html>
)=====";

// ============================================================
// handleRoot(): Handler สำหรับ HTTP GET "/"
// เมื่อมีคนเปิดเบราว์เซอร์มาที่ http://192.168.4.1
// ESP32 จะส่งหน้า HTML ทั้งหมดกลับไปให้
// ============================================================
void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

// ============================================================
// handleData(): Handler สำหรับ HTTP GET "/api/data"
// เมื่อ JavaScript บนมือถือเรียก fetch('/api/data') ทุกวินาที
// ESP32 จะตอบกลับด้วย JSON เช่น {"ppm":0.045}
// ============================================================
void handleData() {
  // สร้าง JSON string แบบ manual (ไม่ต้องใช้ library ArduinoJson)
  // String(currentPPM, 3) = แปลง float เป็น string ทศนิยม 3 ตำแหน่ง
  String json = "{\"ppm\":" + String(currentPPM, 3) + "}";
  server.send(200, "application/json", json);
}

// ============================================================
// setup(): รันครั้งเดียวตอนเปิดเครื่อง — ตั้งค่าทุกอย่าง
// ============================================================
void setup() {
  // เปิด Serial Monitor ที่ 115200 baud สำหรับ Debug
  Serial.begin(115200);
  
  // เปิด Serial2 ที่ 9600 baud สำหรับสื่อสารกับเซนเซอร์
  // SERIAL_8N1 = 8 data bits, No parity, 1 stop bit (มาตรฐาน UART)
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  // ---- ตั้งค่า Wi-Fi แบบ Access Point (AP Mode) ----
  // ESP32 จะทำตัวเป็น Router ให้อุปกรณ์อื่นมาเชื่อมต่อ
  // ต่างจาก STA Mode ที่ ESP32 ไปเชื่อมต่อ Router คนอื่น
  WiFi.softAP(ap_ssid, ap_pass);
  Serial.println("Access Point Started!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP()); // พิมพ์ IP (ปกติคือ 192.168.4.1)

  // ---- ตั้งค่า PIN Mode ----
  pinMode(BUZZER_PIN, OUTPUT); // ขา Buzzer เป็น Output (จ่ายกระแสออก)
  for (int i = 0; i < 10; i++) {
    pinMode(ledPins[i], OUTPUT); // ขา LED ทั้ง 10 ขาเป็น Output
  }

  // ---- กำหนด URL Routes ของ Web Server ----
  server.on("/", handleRoot);          // GET / → ส่ง HTML Dashboard
  server.on("/api/data", handleData);  // GET /api/data → ส่ง JSON ค่า PPM
  server.begin(); // เริ่มต้น Web Server (เริ่มรับ request)
}

// ============================================================
// loop(): รันซ้ำๆ ตลอดเวลาที่เครื่องทำงาน
// มีหน้าที่หลัก 2 อย่าง:
//   1. จัดการ HTTP Clients ที่เชื่อมต่อเข้ามา
//   2. อ่านค่าจากเซนเซอร์และอัปเดต LED/Buzzer
// ============================================================
void loop() {
  // ตรวจสอบและจัดการ HTTP request จาก Client ที่รอคิวอยู่
  // ต้องเรียกบ่อยๆ ไม่ใช่ blocking (ไม่มี delay ระหว่างนี้)
  server.handleClient();

  // ---- อ่านค่าจากเซนเซอร์ทุก 1 วินาที (non-blocking) ----
  // ใช้ millis() แทน delay() เพื่อไม่ให้บล็อก handleClient()
  static unsigned long lastRead = 0; // static = จำค่าระหว่าง loop
  if (millis() - lastRead > 1000) {  // ถ้าผ่านมาแล้ว 1000ms
    lastRead = millis(); // อัปเดตเวลาล่าสุด
    
    // ---- ส่งคำสั่งถามค่าไปยังเซนเซอร์ (Modbus-like Protocol) ----
    // Command ขนาด 9 bytes: Header(FF) + Command(01 86) + Padding(x5) + Checksum(79)
    byte req[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    Serial2.write(req, 9); // ส่ง 9 bytes ออกทาง Serial2 (ไปยังเซนเซอร์)
    
    // ---- รับและถอดรหัสข้อมูลจากเซนเซอร์ ----
    if (Serial2.available() >= 9) { // รอให้มีข้อมูลครบ 9 bytes
      byte res[9];
      Serial2.readBytes(res, 9); // อ่าน response 9 bytes
      
      // ตรวจสอบ Header ว่าถูกต้อง (0xFF 0x86) ก่อนนำค่าไปใช้
      if (res[0] == 0xFF && res[1] == 0x86) {
        // ถอดรหัส PPM จาก byte ที่ 2 (High Byte) และ 3 (Low Byte)
        // สูตร: PPM_raw = (High * 256) + Low, แล้วหาร 1000 → หน่วย PPM
        currentPPM = (res[2] * 256 + res[3]) / 1000.0;
        
        // ---- คำนวณจำนวน LED ที่ต้องติด ----
        // ทุกๆ 0.01 PPM = เพิ่ม LED 1 ดวง (max 10 ดวง)
        int leds = currentPPM / 0.01;
        if (leds > 10) leds = 10; // จำกัดไม่เกิน 10
        
        // เปิด LED ตาม index (0 ถึง leds-1) ปิดที่เหลือ
        for (int i = 0; i < 10; i++) {
          digitalWrite(ledPins[i], (i < leds)); // true=HIGH=เปิด, false=LOW=ปิด
        }
        
        // เปิด Buzzer ถ้าค่าเกิน 0.08 PPM (ระดับอันตราย)
        // HIGH = ส่งสัญญาณให้ Buzzer ดัง, LOW = เงียบ
        digitalWrite(BUZZER_PIN, (currentPPM >= 0.08));
      }
    }
  }
}