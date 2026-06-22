# ESP32 Bluetooth Attack Tool

## Features
1. **BLE Scanner** - Scan semua BT device sekitar (nama, MAC, RSSI, UUID)
2. **BLE Deauth** - Putus koneksi BLE target
3. **BT Jammer** - Flood semua channel 2.4GHz
4. **BT Targeted Jam** - Ganggu specific device aja

## Hardware Required
- ESP32 DevKit V1 (ESP-WROOM-32)
- USB Cable
- PCB antenna bawaan (10-15m range)

## Installation

### 1. Install Arduino IDE
Download: https://www.arduino.cc/en/software

### 2. Add ESP32 Board Support
1. Buka Arduino IDE
2. File → Preferences
3. "Additional Board Manager URLs" tambah:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
4. Tools → Board → Boards Manager
5. Search "ESP32" → Install **esp32 by Espressif**

### 3. Upload Code
1. Buka file `esp32_bt_tool.ino`
2. Tools → Board → DOIT ESP32 DEVKIT V1
3. Tools → Port → Pilih port ESP32
4. Klik Upload (→)

## Usage

### Serial Monitor
1. Tools → Serial Monitor
2. Set baud rate: **115200**
3. Set line ending: **Newline**

### Commands
```
[1] BLE Scanner    - Scan semua device BT sekitar
[2] BLE Deauth     - Putus koneksi BLE target
[3] BT Jammer      - Ganggu semua BT di sekitar
[4] BT Targeted Jam - Ganggu device tertentu
[5] Set Target     - Set MAC address target
[6] Stop Attack    - Stop serangan
[0] Show Menu      - Tampilkan menu
```

### Cara Pakai

#### Step 1: Scan devices
```
Ketik: 1
Enter
```
Tunggu 10 detik, bakal muncul daftar device:
```
[BLE] AA:BB:CC:DD:EE:FF | RSSI: -45 | JBL Flip 5 | UUID: 0x1800
[BLE] 11:22:33:44:55:66 | RSSI: -67 | Mi Band 6
```

#### Step 2: Set target
```
Ketik: 5
Enter
Ketik: AA:BB:CC:DD:EE:FF  (MAC target)
Enter
Ketik: JBL Flip 5  (nama target, optional)
Enter
```

#### Step 3: Attack
```
Ketik: 2  (BLE Deauth)
Enter
```

#### Step 4: Stop
```
Ketik: 6
Enter
```

## Notes

### Range
- **PCB Antenna (bawaan)**: ~10-15m indoor
- **External Antena**: ~30-50m

### Limitasi
- BT 5.0+ devices lebih susah di-deauth
- Speaker premium punya reconnect cepat
- Jarak terbatas tanpa antena external

### Tips
- Deketin target < 5m buat efektif maksimal
- Pakai [4] Targeted Jam buat efisiensi
- LED built-in kedip = attack aktif

## Troubleshooting

### Upload gagal
- Coba tahan tombol BOOT saat upload
- Ganti kabel USB (pastikan data, bukan charge only)
- Pilih port yang benar di Tools → Port

### BLE gak jalan
- Pastikan board = DOIT ESP32 DEVKIT V1
- Bukan ESP32-C3 atau ESP32-S3

### Error compile
- Install library: ESP32 BLE Arduino (sudah include di board package)
- Update board package ke versi terbaru
