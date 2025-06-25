#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>

// Include blockchain interface last (assuming it depends on WiFi)
#include "blockchain_interface.h"

// Forward declarations
class SecuritySystem;
class AuthenticationModule;
class NetworkManager;
class StorageManager;

// ==================== CONFIGURATION ====================
// Pin definitions
#define SS_PIN       5     // RFID SS (SDA)
#define RST_PIN      0     // RFID RST
#define RELAY_PIN    2     // Relay IN (LOW = energize)
#define TILT_PIN     15    // Tilt sensor (INPUT_PULLUP)
#define FINGER_RX    21    // R307 TX → ESP32 RX2
#define FINGER_TX    22    // R307 RX → ESP32 TX2
#define LED_SUCCESS  13    // Green LED for successful authentication
#define LED_ERROR    12    // Red LED for authentication failures
#define BUZZER_PIN   14    // Buzzer for audio feedback

// System parameters
#define UNLOCK_DURATION     30000   // Auto-lock after 30 seconds
#define FP_SCAN_TIMEOUT     10000   // Fingerprint scan timeout (10 seconds)
#define TILT_ALARM_DURATION 30000   // Alarm duration after tilt detection
#define MAX_FAILED_ATTEMPTS 5       // Maximum consecutive failed attempts
#define LOCKOUT_DURATION    300000  // 5-minute lockout after too many failed attempts

// Network retry parameters
#define WIFI_CONNECT_TIMEOUT  20000   // 20 seconds to connect to WiFi
#define MAX_WIFI_RETRIES      5       // Maximum number of WiFi connection attempts
#define BLOCKCHAIN_RETRY      3       // Number of blockchain communication retries

// ==================== STORAGE MANAGER CLASS ====================
class StorageManager {
private:
  Preferences preferences;
  const char* NAMESPACE = "security";
  
public:
  StorageManager() {
    preferences.begin(NAMESPACE, false);
  }
  
  ~StorageManager() {
    preferences.end();
  }
  
  // Securely save network credentials
  void saveNetworkCredentials(const char* ssid, const char* password, const char* serverUrl) {
    preferences.putString("wifi_ssid", ssid);
    preferences.putString("wifi_pass", password);
    preferences.putString("server_url", serverUrl);
  }
  
  // Get network credentials
  bool getNetworkCredentials(String &ssid, String &password, String &serverUrl) {
    ssid = preferences.getString("wifi_ssid", "");
    password = preferences.getString("wifi_pass", "");
    serverUrl = preferences.getString("server_url", "");
    
    return (ssid.length() > 0 && password.length() > 0 && serverUrl.length() > 0);
  }
  
  // Save authorized RFID UIDs
  void saveAuthorizedUID(byte uid[], uint8_t size, uint8_t index) {
    char keyName[20];
    sprintf(keyName, "auth_uid_%d", index);
    preferences.putBytes(keyName, uid, size);
    preferences.putUChar("uid_size", size);
  }
  
  // Get authorized RFID UID by index
  bool getAuthorizedUID(byte uid[], uint8_t &size, uint8_t index) {
    char keyName[20];
    sprintf(keyName, "auth_uid_%d", index);
    size = preferences.getUChar("uid_size", 4);  // Default to 4 if not found
    
    if (size == 0) return false;
    
    size_t readBytes = preferences.getBytes(keyName, uid, size);
    return (readBytes == size);
  }
  
  // Access attempt logging
  void logAccessAttempt(bool success) {
    uint32_t attempts = preferences.getUInt("fail_attempt", 0);
    
    if (success) {
      // Reset failed attempts counter on success
      preferences.putUInt("fail_attempt", 0);
    } else {
      // Increment failed attempts
      preferences.putUInt("fail_attempt", attempts + 1);
    }
  }
  
  uint32_t getFailedAttempts() {
    return preferences.getUInt("fail_attempt", 0);
  }
  
  void resetFailedAttempts() {
    preferences.putUInt("fail_attempt", 0);
  }
};

// ==================== NETWORK MANAGER CLASS ====================
class NetworkManager {
private:
  String ssid;
  String password;
  String serverUrl;
  bool connected;
  uint8_t retryCount;
  BlockchainInterface* blockchain;
  
public:
  NetworkManager() : connected(false), retryCount(0), blockchain(nullptr) {}
  
  bool init(const String &_ssid, const String &_password, const String &_serverUrl) {
    ssid = _ssid;
    password = _password;
    serverUrl = _serverUrl;
    
    // Initialize blockchain interface
    blockchain = new BlockchainInterface(serverUrl.c_str());
    
    return connect();
  }
  
  ~NetworkManager() {
    if (blockchain != nullptr) {
      delete blockchain;
    }
  }
  
  bool connect() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT) {
      delay(500);
      Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      connected = true;
      retryCount = 0;
      return true;
    } else {
      Serial.println("\nFailed to connect to WiFi!");
      connected = false;
      retryCount++;
      return false;
    }
  }
  
  bool isConnected() {
    if (WiFi.status() != WL_CONNECTED) {
      connected = false;
    }
    return connected;
  }
  
  bool ensureConnection() {
    if (!isConnected() && retryCount < MAX_WIFI_RETRIES) {
      return connect();
    }
    return isConnected();
  }
  
  bool logAccessToBlockchain(const char* rfidId, bool accessGranted, const char* fingerprintId) {
    if (!ensureConnection() || blockchain == nullptr) {
      Serial.println("Cannot log to blockchain: No connection");
      return false;
    }
    
    for (int i = 0; i < BLOCKCHAIN_RETRY; i++) {
      if (blockchain->logAccess(rfidId, accessGranted, fingerprintId)) {
        Serial.println("[BLOCKCHAIN] Access logged successfully");
        return true;
      }
      delay(500);
    }
    
    Serial.println("[BLOCKCHAIN] Failed to log access after retries");
    return false;
  }
};

// ==================== AUTHENTICATION MODULE CLASS ====================
class AuthenticationModule {
private:
  MFRC522 rfid;
  Adafruit_Fingerprint finger;
  HardwareSerial fpSerial;
  bool rfidInitialized;
  bool fingerprintInitialized;
  
public:
  AuthenticationModule() : rfid(SS_PIN, RST_PIN), fpSerial(2), finger(&fpSerial), 
                          rfidInitialized(false), fingerprintInitialized(false) {}
  
  bool init() {
    // Initialize RFID reader first (more critical)
    SPI.begin(18, 19, 23, SS_PIN);
    delay(100);
    rfid.PCD_Init();
    delay(100);  // Increased delay for stability
    
    // Set RFID reader for ISO 14443-3A tags
    rfid.PCD_SetAntennaGain(rfid.RxGain_max);
    
    // Verify RFID communication
    byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
    if (version == 0x00 || version == 0xFF) {
      Serial.println("Warning: MFRC522 communication issue - check wiring");
      Serial.print("Version register: 0x");
      Serial.println(version, HEX);
      rfidInitialized = false;
    } else {
      Serial.print("MFRC522 version: 0x");
      Serial.println(version, HEX);
      rfidInitialized = true;
    }
    
    // Try a soft reset of the RFID reader
    rfid.PCD_Reset();
    delay(100);  // Increased delay
    rfid.PCD_Init();
    delay(100);  // Increased delay
    
    // Configure for ISO14443-3A tags
    rfid.PCD_WriteRegister(rfid.TModeReg, 0x80);
    rfid.PCD_WriteRegister(rfid.TPrescalerReg, 0xA9);
    rfid.PCD_WriteRegister(rfid.TReloadRegH, 0x03);
    rfid.PCD_WriteRegister(rfid.TReloadRegL, 0xE8);
    rfid.PCD_WriteRegister(rfid.TxASKReg, 0x40);
    rfid.PCD_WriteRegister(rfid.ModeReg, 0x3D);
    
    // Turn antenna on
    rfid.PCD_AntennaOn();
    
    Serial.println("RFID reader initialized for ISO 14443-3A tags");
    
    // Initialize fingerprint sensor with error tolerance
    // Start serial at a slower rate
    fpSerial.begin(57600, SERIAL_8N1, FINGER_RX, FINGER_TX);
    delay(1000);  // Give more time for serial to initialize
    
    finger.begin(57600);
    delay(500);  // More time for sensor to initialize
    
    // Try several times to verify the fingerprint sensor
    for (int retry = 0; retry < 3; retry++) {
      if (finger.verifyPassword()) {
        Serial.println("Fingerprint sensor initialized");
        fingerprintInitialized = true;
        break;
      }
      delay(500);  // Wait between retries
    }
    
    if (!fingerprintInitialized) {
      Serial.println("WARNING: Fingerprint sensor not found! System will run with RFID only.");
    }
    
    // Return true as long as RFID is working (can operate in degraded mode)
    return rfidInitialized;
  }
  
  bool isRfidCardPresent() {
    return rfid.PICC_IsNewCardPresent();
  }
  
  bool readRfidCard(byte uid[], uint8_t &size) {
    if (!rfid.PICC_ReadCardSerial()) {
      return false;
    }
    
    // Check if card is ISO 14443-3A compliant
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    Serial.print("PICC type: ");
    Serial.println(rfid.PICC_GetTypeName(piccType));
    
    // Copy the UID
    size = rfid.uid.size;
    memcpy(uid, rfid.uid.uidByte, size);
    
    // Print UID for debugging
    Serial.print("RFID Tag detected: ");
    for (byte i = 0; i < size; i++) {
      Serial.printf("%02X", uid[i]);
    }
    Serial.println();
    
    // Clean up RFID reader
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    
    return true;
  }
  
  bool verifyRfidCard(byte uid[], uint8_t size, byte expectedUID[], uint8_t expectedSize) {
    if (size != expectedSize) {
      Serial.println("UID size mismatch");
      return false;
    }
    
    for (byte i = 0; i < size; i++) {
      if (uid[i] != expectedUID[i]) {
        Serial.println("RFID mismatch");
        return false;
      }
    }
    
    Serial.println("RFID match");
    return true;
  }
  
  bool scanFingerprint(uint16_t &fingerprintId) {
    Serial.println("Waiting for fingerprint...");
    
    unsigned long startTime = millis();
    while (millis() - startTime < FP_SCAN_TIMEOUT) {
      uint8_t p = finger.getImage();
      
      if (p == FINGERPRINT_OK) {
        p = finger.image2Tz();
        if (p == FINGERPRINT_OK) {
          p = finger.fingerFastSearch();
          if (p == FINGERPRINT_OK) {
            fingerprintId = finger.fingerID;
            Serial.print("Fingerprint ID #");
            Serial.print(fingerprintId);
            Serial.print(" with confidence ");
            Serial.println(finger.confidence);
            return true;
          } else {
            Serial.println("Finger not found in database");
            return false;
          }
        } else {
          Serial.println("Image conversion failed");
          return false;
        }
      }
      delay(100);
    }
    
    Serial.println("Fingerprint scan timeout");
    return false;
  }
  
  // Enroll a new fingerprint
  int16_t enrollFingerprint(uint16_t id) {
    int p = -1;
    Serial.println("Waiting for valid finger to enroll");
    
    while (p != FINGERPRINT_OK) {
      p = finger.getImage();
      switch (p) {
        case FINGERPRINT_OK:
          Serial.println("Image taken");
          break;
        case FINGERPRINT_NOFINGER:
          Serial.print(".");
          delay(100);
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          Serial.println("Communication error");
          return -1;
        case FINGERPRINT_IMAGEFAIL:
          Serial.println("Imaging error");
          return -1;
        default:
          Serial.println("Unknown error");
          return -1;
      }
    }
    
    p = finger.image2Tz(1);
    if (p != FINGERPRINT_OK) {
      Serial.println("Image conversion failed");
      return -1;
    }
    
    Serial.println("Remove finger");
    delay(2000);
    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
      p = finger.getImage();
    }
    
    Serial.println("Place same finger again");
    p = -1;
    while (p != FINGERPRINT_OK) {
      p = finger.getImage();
      delay(100);
    }
    
    p = finger.image2Tz(2);
    if (p != FINGERPRINT_OK) {
      Serial.println("Image conversion failed");
      return -1;
    }
    
    p = finger.createModel();
    if (p != FINGERPRINT_OK) {
      Serial.println("Could not create model");
      return -1;
    }
    
    p = finger.storeModel(id);
    if (p != FINGERPRINT_OK) {
      Serial.println("Storing model failed");
      return -1;
    }
    
    Serial.print("Fingerprint ID #");
    Serial.print(id);
    Serial.println(" enrolled successfully!");
    return id;
  }
};

// ==================== MAIN SECURITY SYSTEM CLASS ====================
class SecuritySystem {
private:
  // System components
  AuthenticationModule auth;
  NetworkManager network;
  StorageManager storage;
  
  // System state
  bool lockState;
  unsigned long unlockTime;
  bool systemInitialized;
  bool tiltAlarmActive;
  unsigned long tiltAlarmStartTime;
  unsigned long systemLockoutTime;
  
  // Expected tag UID (will be loaded from storage)
  byte expectedUID[10];  // Support up to 10 bytes
  uint8_t expectedUIDSize;
  
  void soundBuzzer(int pattern) {
    switch (pattern) {
      case 0: // Success
        for (int i = 0; i < 2; i++) {
          digitalWrite(BUZZER_PIN, HIGH);
          delay(100);
          digitalWrite(BUZZER_PIN, LOW);
          delay(100);
        }
        break;
      case 1: // Error
        digitalWrite(BUZZER_PIN, HIGH);
        delay(500);
        digitalWrite(BUZZER_PIN, LOW);
        break;
      case 2: // Alert
        for (int i = 0; i < 5; i++) {
          digitalWrite(BUZZER_PIN, HIGH);
          delay(50);
          digitalWrite(BUZZER_PIN, LOW);
          delay(50);
        }
        break;
    }
  }
  
  void updateLEDs() {
    // Update LEDs based on system state
    if (!lockState) {
      digitalWrite(LED_SUCCESS, HIGH);
      digitalWrite(LED_ERROR, LOW);
    } else if (systemLockoutTime > 0) {
      // System in lockout mode - blink error LED
      if ((millis() / 500) % 2 == 0) {
        digitalWrite(LED_ERROR, HIGH);
      } else {
        digitalWrite(LED_ERROR, LOW);
      }
      digitalWrite(LED_SUCCESS, LOW);
    } else {
      digitalWrite(LED_SUCCESS, LOW);
      digitalWrite(LED_ERROR, LOW);
    }
  }
  
public:
  SecuritySystem() : lockState(true), unlockTime(0), systemInitialized(false), 
                     tiltAlarmActive(false), tiltAlarmStartTime(0), systemLockoutTime(0),
                     expectedUIDSize(4) {
    // Set default UID (will be overwritten from storage)
    expectedUID[0] = 0x63;
    expectedUID[1] = 0x5A;
    expectedUID[2] = 0x59;
    expectedUID[3] = 0x31;
  }
  
  bool init() {
    // Initialize pins first
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(TILT_PIN, INPUT_PULLUP);
    pinMode(LED_SUCCESS, OUTPUT);
    pinMode(LED_ERROR, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    
    // Ensure relay starts in locked state
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_SUCCESS, LOW);
    digitalWrite(LED_ERROR, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    
    delay(1000);
    
    // Load credentials or use defaults
    String ssid, password, serverUrl;
    if (!storage.getNetworkCredentials(ssid, password, serverUrl)) {
        // No credentials stored, use defaults
        ssid = "Paul Zion SM-A9";
        password = "whereiswisdom"; // Original password for testing
        serverUrl = "http://192.168.43.230:3000";
        
        // Save for future use
        storage.saveNetworkCredentials(ssid.c_str(), password.c_str(), serverUrl.c_str());
    }
    
    // Load authorized UIDs
    if (!storage.getAuthorizedUID(expectedUID, expectedUIDSize, 0)) {
      // No UID stored, use default
      expectedUID[0] = 0x63;
      expectedUID[1] = 0x5A;
      expectedUID[2] = 0x59;
      expectedUID[3] = 0x31;
      expectedUIDSize = 4;
      
      storage.saveAuthorizedUID(expectedUID, expectedUIDSize, 0);
    }
    
    // Initialize network (non-critical, can continue if fails)
    if (!network.init(ssid, password, serverUrl)) {
      Serial.println("Network initialization failed. System will run in offline mode.");
      // Continue anyway - system can work offline
    }
    
    // Initialize authentication modules
    if (!auth.init()) {
      Serial.println("Authentication system initialization failed!");
      digitalWrite(LED_ERROR, HIGH); // Turn on error LED
      delay(500);
      digitalWrite(LED_ERROR, LOW);
      delay(500);
      digitalWrite(LED_ERROR, HIGH);
      delay(500);
      digitalWrite(LED_ERROR, LOW);
      
      // We'll continue anyway with limited functionality
      Serial.println("Continuing with limited functionality");
    }
    
    systemInitialized = true;
    Serial.println("=== SYSTEM READY ===");
    
    // Short beep to indicate system is ready
    soundBuzzer(0);
    
    return true;
  }
  
  void update() {
    // Check for system lockout first
    if (systemLockoutTime > 0) {
      if (millis() - systemLockoutTime >= LOCKOUT_DURATION) {
        Serial.println("System lockout period ended");
        systemLockoutTime = 0;
        storage.resetFailedAttempts();
      } else {
        // System is in lockout mode, don't process authentication
        updateLEDs();
        return;
      }
    }
    
    // Handle auto-locking based on timer
    if (!lockState && millis() - unlockTime >= UNLOCK_DURATION) {
      lockSystem();
    }
    
    // Check for authentication attempts
    checkAuthentication();
    
    // Check tilt sensor (always active)
    checkTiltSensor();
    
    // Update LEDs based on system state
    updateLEDs();
  }
  
  void checkAuthentication() {
    // Only proceed with authentication if currently locked
    if (!lockState) return;
    
    // Check for too many failed attempts
    if (storage.getFailedAttempts() >= MAX_FAILED_ATTEMPTS) {
      if (systemLockoutTime == 0) {  // Only set lockout time once
        Serial.println("Too many failed attempts! System locked for security.");
        systemLockoutTime = millis();
        soundBuzzer(1);  // Error sound
      }
      return;
    }
    
    // Step 1: Check for RFID card
    if (!auth.isRfidCardPresent()) {
      return;  // No card present
    }
    
    // Try to read the card
    byte cardUID[10];
    uint8_t uidSize;
    
    if (!auth.readRfidCard(cardUID, uidSize)) {
      return;  // Read failure
    }
    
    // Verify card UID
    if (!auth.verifyRfidCard(cardUID, uidSize, expectedUID, expectedUIDSize)) {
      // Failed RFID authentication
      storage.logAccessAttempt(false);
      digitalWrite(LED_ERROR, HIGH);
      soundBuzzer(1);  // Error sound
      delay(1000);
      digitalWrite(LED_ERROR, LOW);
      return;
    }
    
    Serial.println("RFID match. Please place finger...");
    
    // Indicate waiting for fingerprint
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_SUCCESS, HIGH);
      delay(100);
      digitalWrite(LED_SUCCESS, LOW);
      delay(100);
    }
    
    // Step 2: Check fingerprint
    uint16_t fingerprintId = 0;
    bool fingerprint_ok = auth.scanFingerprint(fingerprintId);
    
    if (!fingerprint_ok) {
      // Failed fingerprint authentication
      storage.logAccessAttempt(false);
      digitalWrite(LED_ERROR, HIGH);
      soundBuzzer(1);  // Error sound
      delay(1000);
      digitalWrite(LED_ERROR, LOW);
      return;
    }
    
    // Both authentication succeeded
    storage.logAccessAttempt(true);
    unlockSystem(fingerprintId);
  }
  
  void unlockSystem(uint16_t fingerprintId) {
    Serial.println("Authentication successful. Unlocking...");
    digitalWrite(RELAY_PIN, LOW);  // LOW = energize relay (unlock)
    lockState = false;
    unlockTime = millis();
    
    // Visual and audio feedback
    digitalWrite(LED_SUCCESS, HIGH);
    soundBuzzer(0);  // Success sound
    
    // Format RFID as string
    char rfidStr[32];
    sprintf(rfidStr, "%02X:%02X:%02X:%02X", 
            expectedUID[0], expectedUID[1], expectedUID[2], expectedUID[3]);
    
    // Format fingerprint ID
    char fingerprintStr[8];
    sprintf(fingerprintStr, "%d", fingerprintId);
    
    // Log to blockchain (async - don't wait for response)
    network.logAccessToBlockchain(rfidStr, true, fingerprintStr);
  }
  
  void lockSystem() {
    if (!lockState) {  // Only lock if currently unlocked
      digitalWrite(RELAY_PIN, HIGH);  // HIGH = de-energize relay (lock)
      lockState = true;
      digitalWrite(LED_SUCCESS, LOW);
      Serial.println("System locked.");
    }
  }
  
  void checkTiltSensor() {
    static bool lastTiltState = LOW;
    static bool initialRead = true;
    
    bool currentTiltState = digitalRead(TILT_PIN);
    
    // Skip the first read to avoid false alarms at startup
    if (initialRead) {
      lastTiltState = currentTiltState;
      initialRead = false;
      return;
    }
    
    // Only trigger alarm on state change to avoid flooding
    if (currentTiltState == HIGH && lastTiltState == LOW) {
      Serial.println("[ALERT] Unauthorized Access Attempt Detected!");
      
      // Start alarm
      tiltAlarmActive = true;
      tiltAlarmStartTime = millis();
      
      // Visual and audio feedback
      digitalWrite(LED_ERROR, HIGH);
      soundBuzzer(2);  // Alert sound
      
      // Log tampering attempt to blockchain
      char rfidStr[32] = "TAMPER";
      char fingerprintStr[8] = "0";
      network.logAccessToBlockchain(rfidStr, false, fingerprintStr);
    }
    
    // Handle active alarm
    if (tiltAlarmActive) {
      // Blink error LED and sound alarm periodically
      if ((millis() / 250) % 2 == 0) {
        digitalWrite(LED_ERROR, HIGH);
      } else {
        digitalWrite(LED_ERROR, LOW);
      }
      
      // Sound alarm every 5 seconds
      if ((millis() - tiltAlarmStartTime) % 5000 < 100) {
        soundBuzzer(2);
      }
      
      // Automatically stop alarm after set duration
      if (millis() - tiltAlarmStartTime >= TILT_ALARM_DURATION) {
        tiltAlarmActive = false;
        digitalWrite(LED_ERROR, LOW);
      }
    }
    
    lastTiltState = currentTiltState;
  }
  
  // Admin function to enroll a new fingerprint
  bool enrollNewFingerprint(uint16_t id) {
    return (auth.enrollFingerprint(id) == id);
  }
  
  // Admin function to add a new RFID card
  bool addNewRfidCard(uint8_t index) {
    Serial.println("Place new RFID card to enroll...");
    
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {  // 10 second timeout
      if (auth.isRfidCardPresent()) {
        byte newUID[10];
        uint8_t uidSize;
        
        if (auth.readRfidCard(newUID, uidSize)) {
          // Save to storage
          storage.saveAuthorizedUID(newUID, uidSize, index);
          Serial.println("New RFID card enrolled successfully!");
          return true;
        }
      }
      delay(100);
    }
    
    Serial.println("RFID enrollment timed out.");
    return false;
  }
};

// ==================== GLOBAL VARIABLES ====================
SecuritySystem securitySystem;

// ==================== SETUP & LOOP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);  // Give time for serial to initialize
  
  Serial.println("\n\n=== Security System Starting ===");
  
  if (!securitySystem.init()) {
    Serial.println("ERROR: System initialization failed!");
    // We'll still continue with limited functionality
    Serial.println("Continuing with limited functionality");
  }
}

void loop() {
  securitySystem.update();
  
  // Handle serial commands for administration
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "enroll") {
      Serial.println("Enter fingerprint ID (1-127):");
      while (!Serial.available()) {
        delay(100);
      }
      
      int id = Serial.parseInt();
      if (id > 0 && id < 128) {
        if (securitySystem.enrollNewFingerprint(id)) {
          Serial.println("Fingerprint enrolled successfully!");
        } else {
          Serial.println("Failed to enroll fingerprint.");
        }
      } else {
        Serial.println("Invalid ID. Must be between 1-127");
      }
    } else if (command == "addcard") {
      Serial.println("Enter card index (0-9):");
      while (!Serial.available()) {
        delay(100);
      }
      
      int index = Serial.parseInt();
      if (index >= 0 && index < 10) {
        if (securitySystem.addNewRfidCard(index)) {
          Serial.println("RFID card added successfully!");
        } else {
          Serial.println("Failed to add RFID card.");
        }
      } else {
        Serial.println("Invalid index. Must be between 0-9");
      }
    } else if (command == "lock") {
      securitySystem.lockSystem();
      Serial.println("System manually locked.");
    } else if (command == "status") {
      Serial.println("System Status:");
      Serial.println("-------------");
      Serial.print("WiFi: ");
      Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.print("RSSI: ");
      Serial.println(WiFi.RSSI());
    } else if (command == "help") {
      Serial.println("Available commands:");
      Serial.println("  enroll - Enroll new fingerprint");
      Serial.println("  addcard - Add new RFID card");
      Serial.println("  lock - Manually lock system");
      Serial.println("  status - Show system status");
      Serial.println("  help - Show this help");
    }
  }
  
  delay(100); // Short delay for stability
}