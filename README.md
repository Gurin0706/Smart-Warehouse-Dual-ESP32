# Smart Warehouse Access Control & Environmental Monitoring (Dual-ESP32)

> 📄 **Full Technical Report:** [Read Project Report (PDF)](docs/Smart_Warehouse_Report.pdf)  
> 🎥 **Hardware Demonstration:** [Watch System Demo Video](https://drive.google.com/file/d/1EAwvYdbpp2mIZKpxihGwusgRcvrklwWR/view?usp=sharing)

## 1. Overview
This project implements a **Smart Warehouse Management System** powered by a **Dual-ESP32 (Master-Slave) architecture** communicating over **UART**[cite: 65]. The system integrates multi-factor access control (RFID & Keypad PIN), non-volatile credential storage (EEPROM), real-time environmental & gas hazard monitoring, and ultrasonic warehouse capacity tracking[cite: 65].

---

## 2. System Architecture

```mermaid
graph LR
    subgraph Master_Node [ESP32 Master - Access & Actuation]
        RFID[MFRC522 RFID - SPI] --> M_ESP[ESP32 Master]
        KP[4x4 Matrix Keypad] --> M_ESP
        M_ESP <--> EEP[(Internal EEPROM)]
        M_ESP --> LCD[LCD 16x2 - I2C]
        M_ESP --> SERVO[Servo SG90 - PWM]
        M_ESP --> RELAY[Relay & Buzzer Alarm]
        M_ESP --> RLED[Auto Room Lighting]
    end

    subgraph Slave_Node [ESP32 Slave - Telemetry & Safety]
        DHT[DHT11 Temp/Humidity] --> S_ESP[ESP32 Slave]
        MQ2[MQ-2 Gas Sensor] --> S_ESP
        US[HC-SR04 Ultrasonic] --> S_ESP
        RTC[DS3231 RTC - I2C] <--> S_ESP
        S_ESP --> OLED[0.96" OLED - I2C]
        S_ESP --> CLED[Capacity LEDs: G/Y/R]
    end

    M_ESP <==>|UART2 9600bps| S_ESP
```

---

## 3. Key Features
* **Dual-Mode Authentication:** Supports 13.56MHz RFID card scanning (up to 50 cards) and 5-digit PIN entry via a 4x4 keypad[cite: 65].
* **On-Device Admin Management:** Add/delete individual RFID tags (`*101#`, `*102#`), wipe all tags (`*103#`), change/reset passwords (`*#01#`, `*#02#`), and perform hard resets (`*#03#`) directly from the keypad without reflashing firmware[cite: 65].
* **Security Lockout & Occupancy Tracking:** Locks input for 15 seconds with audible buzzer alerts after 3 consecutive failed attempts; automatically toggles room lighting based on entry/exit card state[cite: 65].
* **Real-Time Safety & Telemetry:** Monitors ambient temperature, humidity, and flammable gas leaks (MQ-2)[cite: 65]. Triggers immediate cross-MCU UART interrupts (`GAS_ON`) to activate the alarm relay[cite: 65].
* **Warehouse Capacity Indicator:** Measures remaining storage depth using an HC-SR04 ultrasonic sensor and indicates capacity levels via Green (`> 13.4 cm`), Yellow (`6.8–13.4 cm`), and Red (`<= 6.8 cm`) LEDs[cite: 65].

---

## 4. Inter-MCU UART Protocol (`Serial2` @ 9600 bps)

| Direction | Command / Payload | Trigger Event & Action |
| :--- | :--- | :--- |
| **Master $\rightarrow$ Slave** | `DOOR_OPEN` | Sent upon valid RFID/PIN authentication; Slave logs timestamp from DS3231 onto OLED[cite: 65]. |
| **Slave $\rightarrow$ Master** | `TIME_OPEN: hh:mm:ss dd/mm/yyyy` | Confirmation timestamp sent back to Master for serial logging[cite: 65]. |
| **Slave $\rightarrow$ Master** | `GAS_ON` | Triggered when MQ-2 detects gas leak; Master activates Relay alarm[cite: 65]. |
| **Slave $\rightarrow$ Master** | `GAS_OFF` | Triggered when gas levels return to normal; Master deactivates Relay[cite: 65]. |

---

## 5. Hardware Pinout Summary

### ESP32 Master
| Peripheral | Interface | ESP32 GPIO Pins |
| :--- | :--- | :--- |
| **RFID MFRC522** | SPI | `SS=5`, `RST=4`, `SCK=18`, `MOSI=23`, `MISO=19`[cite: 65] |
| **Keypad 4x4** | Digital I/O | Rows: `13, 12, 14, 27` \| Cols: `26, 25, 33, 32`[cite: 65] |
| **LCD 1602** | I2C (`0x27`) | `SDA=21`, `SCL=22`[cite: 65] |
| **Servo SG90** | PWM (50Hz) | `GPIO 2`[cite: 65] |
| **Alarm Buzzer / Relay / Room LED** | Digital Out | Buzzer: `GPIO 15` \| Relay: `GPIO 3` \| Room LED: `GPIO 0`[cite: 65] |
| **UART2 (to Slave)** | UART | `RX2=16`, `TX2=17`[cite: 65] |

### ESP32 Slave
| Peripheral | Interface | ESP32 GPIO Pins |
| :--- | :--- | :--- |
| **OLED SSD1306 & RTC DS3231** | I2C (`0x3C` / `0x68`) | `SDA=21`, `SCL=22`[cite: 65] |
| **DHT11 Sensor** | Single-Wire | `GPIO 4`[cite: 65] |
| **MQ-2 Gas Sensor** | Analog / Digital | `AO=35`, `DO=34`[cite: 65] |
| **Ultrasonic HC-SR04** | Digital I/O | `TRIG=14`, `ECHO=27`[cite: 65] |
| **Capacity Status LEDs** | Digital Out | Green: `25`, Yellow: `26`, Red: `33`[cite: 65] |
| **UART2 (to Master)** | UART | `RX2=16`, `TX2=17`[cite: 65] |

---

## 6. Repository Structure

```text
Smart-Warehouse-Dual-ESP32/
├── firmware/
│   ├── esp32_master/
│   │   └── esp32_master.ino       # Access control, EEPROM management & UI logic
│   └── esp32_slave/
│       └── esp32_slave.ino        # Sensor acquisition, RTC logging & OLED telemetry
├── docs/
│   └── Smart_Warehouse_Report.pdf # Detailed hardware schematics & experimental results
└── README.md
```