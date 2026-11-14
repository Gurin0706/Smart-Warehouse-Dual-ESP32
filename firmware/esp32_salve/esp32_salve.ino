#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <DHT.h>

// --- CẤU HÌNH CHÂN KẾT NỐI (Theo Bảng 3.1 & Sơ đồ mạch) ---
#define DHTPIN 4
#define DHTTYPE DHT11          // Đổi thành DHT22 nếu dùng cảm biến DHT22

#define MQ2_AO_PIN 35
#define MQ2_DO_PIN 34

#define TRIG_PIN 14
#define ECHO_PIN 27

#define LED_RED_PIN 33         // Kho đầy (<= 6.8 cm)
#define LED_YELLOW_PIN 26      // Kho trung bình (6.8 - 13.4 cm)
#define LED_GREEN_PIN 25       // Kho còn trống (> 13.4 cm)

#define UART_RX_PIN 16         // Nối với TX2 (GPIO 17) của ESP32 chính
#define UART_TX_PIN 17         // Nối với RX2 (GPIO 16) của ESP32 chính

// --- KHỞI TẠO ĐỐI TƯỢNG ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS3231 rtc;
DHT dht(DHTPIN, DHTTYPE);

// --- BIẾN TOÀN CỤC ---
float temperature = 0.0;
float humidity = 0.0;
String lastDoorOpenTime = "--:--:--";
bool isGasAlert = false;
unsigned long lastSensorRead = 0;

// Hàm đo khoảng cách bằng HC-SR04 (cm)
float getDistanceCM() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout 30ms
    if (duration == 0) return 999.0;
    return duration * 0.034 / 2.0;
}

// Hàm cập nhật LED trạng thái sức chứa kho (Theo Lưu đồ Hình 3.6)
void updateWarehouseCapacity() {
    float distance = getDistanceCM();

    if (distance > 13.4) {
        digitalWrite(LED_GREEN_PIN, HIGH);
        digitalWrite(LED_YELLOW_PIN, LOW);
        digitalWrite(LED_RED_PIN, LOW);
    } else if (distance > 6.8 && distance <= 13.4) {
        digitalWrite(LED_GREEN_PIN, LOW);
        digitalWrite(LED_YELLOW_PIN, HIGH);
        digitalWrite(LED_RED_PIN, LOW);
    } else {
        digitalWrite(LED_GREEN_PIN, LOW);
        digitalWrite(LED_YELLOW_PIN, LOW);
        digitalWrite(LED_RED_PIN, HIGH);
    }
}

// Hàm giám sát khí gas MQ-2 và gửi cảnh báo UART về ESP32 chính
void checkGasSensor() {
    // MQ-2 DO thường tích cực mức THẤP (0) khi phát hiện khí gas vượt ngưỡng
    bool gasDetected = (digitalRead(MQ2_DO_PIN) == LOW);

    if (gasDetected && !isGasAlert) {
        isGasAlert = true;
        Serial2.println("GAS_ON");
        Serial.println("Canh bao: Phat hien khi gas -> Gui GAS_ON");
    } else if (!gasDetected && isGasAlert) {
        isGasAlert = false;
        Serial2.println("GAS_OFF");
        Serial.println("An toan: Het khi gas -> Gui GAS_OFF");
    }
}

// Hàm nhận lệnh từ ESP32 chính qua UART2
void checkMasterCommand() {
    if (Serial2.available()) {
        String cmd = Serial2.readStringUntil('\n');
        cmd.trim();

        if (cmd == "DOOR_OPEN") {
            DateTime now = rtc.now();
            char timeBuff[10];
            char fullTimeBuff[25];

            sprintf(timeBuff, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
            lastDoorOpenTime = String(timeBuff);

            sprintf(fullTimeBuff, "TIME_OPEN: %02d:%02d:%02d %02d/%02d/%04d",
                    now.hour(), now.minute(), now.second(),
                    now.day(), now.month(), now.year());
            Serial2.println(fullTimeBuff);
            Serial.println(fullTimeBuff);
        }
    }
}

// Hàm hiển thị thông số lên màn hình OLED (Theo Hình 4.5)
void updateOLED() {
    DateTime now = rtc.now();
    char timeStr[16];
    char dateStr[16];

    sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    sprintf(dateStr, "%02d/%02d/%04d", now.day(), now.month(), now.year());

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.print("Gio: ");
    display.println(timeStr);

    display.setCursor(0, 12);
    display.print("Ngay: ");
    display.println(dateStr);

    display.setCursor(0, 24);
    display.print("Nhiet do: ");
    display.print(temperature, 2);
    display.println(" C");

    display.setCursor(0, 36);
    display.print("Do am: ");
    display.print(humidity, 2);
    display.println(" %");

    display.setCursor(0, 48);
    display.print("Cua mo: ");
    display.println(lastDoorOpenTime);

    display.display();
}

void setup() {
    Serial.begin(9600);
    Serial2.begin(9600, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    pinMode(MQ2_DO_PIN, INPUT);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, LOW);

    dht.begin();
    Wire.begin(21, 22);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Khong tim thay man hinh OLED!");
    }
    display.clearDisplay();
    display.display();

    if (!rtc.begin()) {
        Serial.println("Khong tim thay RTC DS3231!");
    }
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

void loop() {
    checkMasterCommand();
    checkGasSensor();

    // Đọc DHT11, siêu âm và cập nhật OLED mỗi 1 giây
    if (millis() - lastSensorRead >= 1000) {
        lastSensorRead = millis();

        float t = dht.readTemperature();
        float h = dht.readHumidity();
        if (!isnan(t) && !isnan(h)) {
            temperature = t;
            humidity = h;
        }

        updateWarehouseCapacity();
        updateOLED();
    }
}