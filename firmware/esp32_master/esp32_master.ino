#include <Arduino.h>
#include <LiquidCrystal_I2C.h> //Thu vien lcd
#include <Keypad.h>
#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

#define PIN_SG90 2
#define SS_PIN 5  // ESP32 pin GIOP5
#define RST_PIN 4 // ESP32 pin GIOP27
#define PIN_BUZZER 15 
#define RELAY_PIN  3  // Kích relay cho buzzer
#define LED_ROOM_PIN 0  // LED phòng dùng chân GPIO0

typedef struct {
  byte id[4];
  bool isInside;
} RFIDCard;

RFIDCard cards[50];   // Tối đa 50 thẻ
int cardCount = 0;
int peopleInRoom = 0;


typedef enum
{
    MODE_ID_RFID_ADD,
    MODE_ID_RFID_FIRST,
    MODE_ID_RFID_SECOND,
} MODE_ID_RFID_E;

unsigned char MODE_RFID = MODE_ID_RFID_ADD;

unsigned char id = 0;
unsigned char id_rf = 0;
unsigned char index_t = 0;
unsigned char error_in = 0;

// init keypad
const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns

char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {13, 12, 14, 27}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {26, 25, 33, 32}; // connect to the column pinouts of the keypad

int addr = 0;
char password[6] = "33333";
char pass_def[6] = "12345";
char mode_changePass[6] = "*#01#";
char mode_resetPass[6] = "*#02#";
char mode_hardReset[6] = "*#03#";
char mode_addRFID[6] = "*101#";
char mode_delRFID[6] = "*102#";
char mode_delAllRFID[6] = "*103#";

char data_input[6];
char new_pass1[6];
char new_pass2[6];

unsigned char in_num = 0, error_pass = 0, isMode = 0;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
MFRC522 rfid(SS_PIN, RST_PIN);
Servo sg90;

void triggerBuzzer(int duration){
    digitalWrite(PIN_BUZZER, HIGH);
    delay(duration);
    digitalWrite(PIN_BUZZER, LOW);

}

void writeEpprom(char data[])
{
    unsigned char i = 0;
    for (i = 0; i < 5; i++)
    {
        EEPROM.write(i, data[i]);
    }
    EEPROM.commit();
}

void readEpprom()
{
    unsigned char i = 0;
    for (i = 0; i < 5; i++)
    {
        password[i] = EEPROM.read(i);
    }
}

void clear_data_input() // xoa gia tri nhap vao hien tai
{
    int i = 0;
    for (i = 0; i < 6; i++)
    {
        data_input[i] = '\0';
    }
}

unsigned char isBufferdata(char data[]) // Kiem tra buffer da co gia tri chua
{
    unsigned char i = 0;
    for (i = 0; i < 5; i++)
    {
        if (data[i] == '\0')
        {
            return 0;
        }
    }
    return 1;
}

bool compareData(char data1[], char data2[]) // Kiem tra 2 cai buffer co giong nhau hay khong
{
    unsigned char i = 0;
    for (i = 0; i < 5; i++)
    {
        if (data1[i] != data2[i])
        {
            return false;
        }
    }
    return true;
}

void insertData(char data1[], char data2[]) // Gan buffer 2 cho buffer 1
{
    unsigned char i = 0;
    for (i = 0; i < 5; i++)
    {
        data1[i] = data2[i];
    }
}

void getData() // Nhan buffer tu ban phim
{
    char key = keypad.getKey(); // Doc gia tri ban phim
    if (key)
    {
        triggerBuzzer(50); // phát tiếng "tít" ngắn khi nhấn phím
        delay(100);
        // Serial.println("key != 0");
        if (in_num == 0)
        {
            data_input[0] = key;
            lcd.setCursor(5, 1);
            lcd.print(data_input[0]);
            delay(200);
            lcd.setCursor(5, 1);
            lcd.print("*");
        }
        if (in_num == 1)
        {
            data_input[1] = key;
            lcd.setCursor(6, 1);
            lcd.print(data_input[1]);
            delay(200);
            lcd.setCursor(6, 1);
            lcd.print("*");
        }
        if (in_num == 2)
        {
            data_input[2] = key;
            lcd.setCursor(7, 1);
            lcd.print(data_input[2]);
            delay(200);
            lcd.setCursor(7, 1);
            lcd.print("*");
        }
        if (in_num == 3)
        {
            data_input[3] = key;
            lcd.setCursor(8, 1);
            lcd.print(data_input[3]);
            delay(200);
            lcd.setCursor(8, 1);
            lcd.print("*");
        }
        if (in_num == 4)
        {
            data_input[4] = key;
            lcd.setCursor(9, 1);
            lcd.print(data_input[4]);
            delay(200);
            lcd.setCursor(9, 1);
            lcd.print("*");
        }
        if (in_num == 4)
        {
            Serial.println(data_input);
            in_num = 0;
        }
        else
        {
            in_num++;
        }
    }
}

void checkPass() // kiem tra password
{
    getData();
    if (isBufferdata(data_input))
    {
        if (compareData(data_input, password)) // Dung pass
        {
            lcd.clear();
            delay(1000); // Để LED sáng một lúc
            clear_data_input();
            index_t = 3;
    
        }
        else if (compareData(data_input, mode_changePass))
        {
            // Serial.print("mode_changePass");
            lcd.clear();
            clear_data_input();
            index_t = 1;
        }
        else if (compareData(data_input, mode_resetPass))
        {
            // Serial.print("mode_resetPass");
            lcd.clear();
            clear_data_input();
            index_t = 2;
        }
        else if (compareData(data_input, mode_hardReset))
        {
            lcd.setCursor(0, 0);
            lcd.print("---HardReset---");
            writeEpprom(pass_def);
            insertData(password, pass_def);
            clear_data_input();
            delay(2000);
            lcd.clear();
            index_t = 0;
        }
        else if (compareData(data_input, mode_addRFID))
        {
            lcd.clear();
            clear_data_input();
            index_t = 8;
        }
        else if (compareData(data_input, mode_delRFID))
        {
            lcd.clear();
            clear_data_input();
            index_t = 9;
        }
        else if (compareData(data_input, mode_delAllRFID))
        {
            lcd.clear();
            clear_data_input();
            index_t = 10;
        }
        else
        {
            if (error_pass == 2)
            {
                clear_data_input();
                lcd.clear();
                index_t = 4;
            }
            Serial.print("Error");
            lcd.clear();
            lcd.setCursor(1, 1);
            lcd.print("WRONG PASSWORD");
            for (int i = 0; i < 2; i++) {
    triggerBuzzer(200);
    delay(150);  // ngắt giữa 2 tiếng
}
            clear_data_input();
            error_pass++;
            delay(1000);
            lcd.clear();
        }
    }
}

void openDoor()
{
    // Serial.println("Open The Door");
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("---OPENDOOR---");
    unsigned char pos;
    delay(1000);
    Serial2.println("DOOR_OPEN");
    sg90.write(90);
    delay(5000);
    sg90.write(0);
    lcd.clear();
    index_t = 0;
}

void error()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WRONG 3 TIMES");
    delay(2000);
    lcd.setCursor(0, 1);
    lcd.print("Wait 15 sec");

    for (int i = 15; i > 0; i--)
    {
        char buff[3];
        sprintf(buff, "%.2d", i);
        lcd.setCursor(13, 1); // Hiển thị số giây ở cuối dòng
        lcd.print("  ");      // Xóa số cũ
        lcd.setCursor(13, 1);
        lcd.print(buff);
        delay(1000);          // Đợi 1 giây
    }

    lcd.clear();
    index_t = 0;
}


void changePass() // Thay doi pass
{
    lcd.setCursor(0, 0);
    lcd.print("-- Change Pass --");
    delay(3000);
    lcd.setCursor(0, 0);
    lcd.print("--- New Pass ---");
    while (1)
    {
        getData();
        if (isBufferdata(data_input))
        {
            insertData(new_pass1, data_input);
            // Serial.println(new_pass1);
            clear_data_input();
            break;
        }
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("---- AGAIN ----");
    while (1)
    {
        getData();
        if (isBufferdata(data_input))
        {
            insertData(new_pass2, data_input);
            // Serial.println(new_pass2);
            clear_data_input();
            break;
        }
    }
    delay(1000);
    if (compareData(new_pass1, new_pass2))
    {
        lcd.clear();
        // Serial.println("Success");
        lcd.setCursor(0, 0);
        lcd.print("--- Success ---");
        delay(1000);
        writeEpprom(new_pass2);
        insertData(password, new_pass2);
        lcd.clear();
        index_t = 0;
    }
    else
    {
        lcd.clear();
        // Serial.println("miss");
        lcd.setCursor(0, 0);
        lcd.print("-- Mismatched --");
        delay(1000);
        lcd.clear();
        index_t = 0;
    }
}

void resetPass()
{
    unsigned char choise = 0;
    // Serial.println("Pass reset");
    lcd.setCursor(0, 0);
    lcd.print("---Reset Pass---");
    getData();
    if (isBufferdata(data_input))
    {
        if (compareData(data_input, password))
        {
            lcd.clear();
            clear_data_input();
            while (1)
            {
                lcd.setCursor(0, 0);
                lcd.print("---Reset Pass---");
                char key = keypad.getKey();
                if (choise == 0)
                {
                    lcd.setCursor(0, 1);
                    lcd.print(">");
                    lcd.setCursor(2, 1);
                    lcd.print("YES");
                    lcd.setCursor(9, 1);
                    lcd.print(" ");
                    lcd.setCursor(11, 1);
                    lcd.print("NO");
                }
                if (choise == 1)
                {
                    lcd.setCursor(0, 1);
                    lcd.print(" ");
                    lcd.setCursor(2, 1);
                    lcd.print("YES");
                    lcd.setCursor(9, 1);
                    lcd.print(">");
                    lcd.setCursor(11, 1);
                    lcd.print("NO");
                }
                if (key == '*')
                {
                    if (choise == 1)
                    {
                        choise = 0;
                    }
                    else
                    {
                        choise++;
                    }
                }
                if (key == '#' && choise == 0)
                {
                    lcd.clear();
                    delay(1000);
                    writeEpprom(pass_def);
                    insertData(password, pass_def);
                    lcd.setCursor(0, 0);
                    lcd.print("---Reset ok---");
                    delay(1000);
                    lcd.clear();
                    break;
                }
                if (key == '#' && choise == 1)
                {
                    lcd.clear();
                    break;
                }
            }
            index_t = 0;
        }
        else
        {
            index_t = 0;
            lcd.clear();
        }
    }
}

unsigned char numberInput()
{
    char number[5];
    char count_i = 0;
    while (count_i < 2)
    {
        char key = keypad.getKey();
        if (key && key != 'A' && key != 'B' && key != 'C' && key != 'D' && key != '*' && key != '#')
        {
            delay(100);
            lcd.setCursor(10 + count_i, 1);
            lcd.print(key);
            number[count_i] = key;
            count_i++;
        }
    }
    return (number[0] - '0') * 10 + (number[1] - '0');
}

bool isAllowedRFIDTag(byte tag[])
{
    int count = 0;
    for (int i = 10; i < 512; i += 4)
    {
        Serial.print("EEPROM: ");
        for (int j = 0; j < 4; j++)
        {
            Serial.print(EEPROM.read(i + j), HEX);
            if (tag[j] == EEPROM.read(i + j))
            {
                count++;
            }
        }
        Serial.println();
        if (count == 4)
        {
            return true;
        }
        count = 0;
    }
    return false;
}
void processCard(byte scannedID[4]) {
    bool found = false;
  
    for (int i = 0; i < cardCount; i++) {
      if (memcmp(cards[i].id, scannedID, 4) == 0) {
        found = true;
        if (!cards[i].isInside) {
          cards[i].isInside = true;
          peopleInRoom++;
          Serial.println("Vào phòng | Số người: " + String(peopleInRoom));
          digitalWrite(LED_ROOM_PIN, HIGH); // Bật đèn
        } else {
          cards[i].isInside = false;
          peopleInRoom--;
          if (peopleInRoom <= 0) peopleInRoom = 0;
          Serial.println("Ra khỏi phòng | Số người: " + String(peopleInRoom));
          if (peopleInRoom == 0) {
            digitalWrite(LED_ROOM_PIN, LOW); // Tắt đèn
          }
        }
        break;
      }
    }
  
    if (!found) {
      // Thẻ chưa từng add trước đó → thêm tự động
      memcpy(cards[cardCount].id, scannedID, 4);
      cards[cardCount].isInside = true;
      cardCount++;
      peopleInRoom++;
      Serial.println("Thẻ mới → vào phòng | Số người: " + String(peopleInRoom));
      digitalWrite(LED_ROOM_PIN, HIGH); // Bật đèn
    }
  }

void rfidCheck()
{
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
    {
        byte rfidTag[4];
        Serial.print("RFID TAG: ");
        for (byte i = 0; i < rfid.uid.size; i++)
        {
            rfidTag[i] = rfid.uid.uidByte[i];
            Serial.print(rfidTag[i], HEX);
        }
        Serial.println();

        if (isAllowedRFIDTag(rfidTag))
        {
            processCard(rfidTag);  // Cập nhật trạng thái phòng và bật/tắt LED
            lcd.clear();
            delay(1000);
            index_t = 3;
        }
        else
        {
            if (error_pass == 2)
            {
                lcd.clear();
                index_t = 4;
            }
            Serial.print("Error\n");
            lcd.clear();
            lcd.setCursor(3, 1);
            lcd.print("WRONG RFID");
            for (int i = 0; i < 2; i++) {
              triggerBuzzer(500);
              delay(200);
            }
            error_pass++;
            delay(1000);
            lcd.clear();
        }
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
}


void addRFID()
{
    static bool displayed_addnew = false; // Hiển thị dòng "ADD NEW RFID"
    static bool displayed = false;        // Hiển thị hướng dẫn từng bước

    switch (MODE_RFID)
    {
    case MODE_ID_RFID_ADD:
    {
        if (!displayed_addnew)
        {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("  ADD NEW RFID  ");
            delay(1500);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Enter ID (01-50)");
            lcd.setCursor(0, 1);
            lcd.print("ID: ");
            displayed_addnew = true;
        }

        id_rf = numberInput(); // Nhập ID RFID

        if (id_rf < 1 || id_rf > 50)
        {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("   INVALID ID   ");
            delay(2000);
            lcd.clear();
            index_t = 0; // Thoát về màn hình chính
            displayed_addnew = false;
        }
        else
        {
            MODE_RFID = MODE_ID_RFID_FIRST;
            displayed = false;
        }
    }
    break;

    case MODE_ID_RFID_FIRST:
    {
        if (!displayed)
        {
            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print("   Put RFID    ");
            displayed = true;
        }

        if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
        {
            byte rfidTag[4];
            Serial.print("RFID TAG: ");
            for (byte i = 0; i < rfid.uid.size; i++)
            {
                rfidTag[i] = rfid.uid.uidByte[i];
                Serial.print(rfidTag[i], HEX);
            }
            Serial.println();

            if (isAllowedRFIDTag(rfidTag))
            {
                lcd.clear();
                lcd.setCursor(1, 1);
                lcd.print("RFID EXISTS");
                delay(2000);
                lcd.clear();
                index_t = 0;
                MODE_RFID = MODE_ID_RFID_ADD;
                displayed_addnew = false;
                displayed = false;
            }
            else
            {
                MODE_RFID = MODE_ID_RFID_SECOND;
                displayed = false;
            }

            rfid.PICC_HaltA();
            rfid.PCD_StopCrypto1();
        }
    }
    break;

    case MODE_ID_RFID_SECOND:
    {
        if (!displayed)
        {
            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print("  Put Again   ");
            displayed = true;
        }

        if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
        {
            byte rfidTag[4];
            Serial.print("RFID TAG: ");
            for (byte i = 0; i < rfid.uid.size; i++)
            {
                rfidTag[i] = rfid.uid.uidByte[i];
                Serial.print(rfidTag[i], HEX);
            }
            Serial.println();

            // Ghi vào EEPROM tại vị trí ID được chọn
            for (int i = 0; i < 4; i++)
            {
                EEPROM.write(10 + (id_rf - 1) * 4 + i, rfidTag[i]);
            }
            EEPROM.commit();

            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print("Add RFID Done");
            delay(2000);
            lcd.clear();

            index_t = 0;
            MODE_RFID = MODE_ID_RFID_ADD;
            displayed_addnew = false;
            displayed = false;

            rfid.PICC_HaltA();
            rfid.PCD_StopCrypto1();
        }
    }
    break;
    }
}

void delRFID()
{
    char buffDisp[20];
    lcd.setCursor(1, 0);
    lcd.print("  DELETE RFID   ");
    Serial.print("DEL_IN");
    lcd.setCursor(0, 1);
    lcd.print("Input ID: ");
    id_rf = numberInput();
    if (id_rf == 0)
    { // ID #0 not allowed, try again!
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("ID ERROR");
        delay(2000);
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            EEPROM.write(10 + (id_rf - 1) * 4 + i, '\0');
            EEPROM.commit();
            Serial.println(EEPROM.read(10 + (id_rf - 1) * 4 + i), HEX);
        }
        sprintf(buffDisp, "Clear id:%d Done", id_rf);
        lcd.setCursor(0, 1);
        lcd.print(buffDisp);
        Serial.print("DEL_OUT");
        delay(2000);
        lcd.clear();
        index_t = 0;
    }
}

void delAllRFID()
{
    char key = keypad.getKey();
    lcd.setCursor(0, 0);
    lcd.print("CLEAR ALL RFID?");
    if (key == '*')
    {
        isMode = 0;
    }
    if (key == '#')
    {
        isMode = 1;
    }
    if (isMode == 0)
    {
        lcd.setCursor(0, 1);
        lcd.print("> Yes      No  ");
    }
    if (isMode == 1)
    {
        lcd.setCursor(0, 1);
        lcd.print("  Yes    > No  ");
    }
    if (key == '0' && isMode == 0)
    {
        for (int i = 10; i < 512; i++)
        {
            EEPROM.write(i, '\0');
            EEPROM.commit();
            Serial.println(EEPROM.read(i), HEX);
        }
        lcd.setCursor(0, 1);
        lcd.print("  Clear done  ");
        delay(2000);
        index_t = 0;
        lcd.clear();
    }
    if (key == '0' && isMode == 1)
    {
        lcd.clear();
        index_t = 0;
    }
}

void clearEEPROM() {
    for (int i = 0; i < 512; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    Serial.println("EEPROM cleared!");
}

void checkSlaveWarning()
{
  if (Serial2.available()) {
    String cmd = Serial2.readStringUntil('\n');
    cmd.trim();
    Serial.print("Nhan tu Slave: ");
    Serial.println(cmd);  // <-- IN RA TẤT CẢ LỆNH, kể cả TIME_OPEN

    if (cmd == "GAS_ON") {
      digitalWrite(RELAY_PIN, HIGH); // Giữ relay bật
      Serial.println("Relay ON");
    }
    else if (cmd == "GAS_OFF") {
      digitalWrite(RELAY_PIN, LOW);  // Tắt relay
      Serial.println("Relay OFF");
    }
  }
}
void setup()
{
    Serial.begin(9600);
    Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17 (chỉnh tùy bạn)
    EEPROM.begin(512);
    sg90.setPeriodHertz(50);
    sg90.attach(PIN_SG90, 500, 2400);
    SPI.begin();
    rfid.PCD_Init();
    lcd.init();
    lcd.backlight();
    lcd.print("   SYSTEM INIT   ");
    readEpprom();
    delay(2000);
    lcd.clear();

    pinMode(RELAY_PIN, OUTPUT);      // Thiết lập chân relay là OUTPUT
    digitalWrite(RELAY_PIN, LOW);   // Đảm bảo relay TẮT ban đầu
    pinMode(PIN_BUZZER, OUTPUT); // Cấu hình chân buzzer là output
    digitalWrite(PIN_BUZZER, LOW);
    Serial.print("PASSWORK: ");
    Serial.println(password);

    pinMode(LED_ROOM_PIN, OUTPUT);
    digitalWrite(LED_ROOM_PIN, LOW); // Tắt đèn lúc khởi động
}

void loop()
{
    lcd.setCursor(1, 0);
    lcd.print("Enter Password");
    checkPass();
    rfidCheck();

    while (index_t == 1)
    {
        changePass();
    }

    while (index_t == 2)
    {
        resetPass();
    }

    while (index_t == 3)
    {
        openDoor();
        error_pass = 0;
    }

    while (index_t == 4)
    {
        error();
        error_pass = 0;
    }

    while (index_t == 8)
    {
        addRFID();
    }

    while (index_t == 9)
    {
        delRFID();
    }

    while (index_t == 10)
    {
        delAllRFID();
    }
    checkSlaveWarning(); // Kiểm tra dữ liệu từ ESP phụ
}
