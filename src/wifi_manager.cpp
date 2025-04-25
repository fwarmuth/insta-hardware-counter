#include "wifi_manager.h"

/**
 * @brief Lists all files in SPIFFS root directory
 */
void printSpiffsFiles() {
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    Serial.println("Files in SPIFFS:");
    while(file) {
        Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
        file = root.openNextFile();
    }
}

/**
 * @brief Safely copies string to buffer with size checking
 * 
 * @param dest Destination buffer
 * @param source Source string
 * @param maxSize Maximum buffer size
 * @return True if successful
 */
bool copyToBuffer(char* dest, String source, size_t maxSize) {
    size_t len = source.length();
    
    if (len >= maxSize) {
        len = maxSize - 1;
    }
    
    memset(dest, 0, maxSize);  // Clear buffer first
    memcpy(dest, source.c_str(), len);
    dest[len] = '\0';  // Ensure null termination
    
    return true;
}

/**
 * @brief Logs credential information for debugging
 */
void logCredentials(const char* ssid, const char* password) {
    size_t ssidLen = strlen(ssid);
    size_t pwdLen = strlen(password);
    
    Serial.println("WiFi credentials loaded from config file");
    Serial.printf("SSID: [%s]\n", ssid);
    Serial.printf("SSID length: %d\n", ssidLen);
    Serial.printf("Password: [%s]\n", password);
    Serial.printf("Password length: %d\n", pwdLen);
    
    Serial.println("SSID hex values:");
    for (size_t i = 0; i < ssidLen; i++) {
        Serial.printf("0x%02X ", (uint8_t)ssid[i]);
    }
    Serial.println();
}

/**
 * @brief Attempts to connect to WiFi using given credentials
 * 
 * @param ssid WiFi network SSID
 * @param password WiFi network password
 * @return True if connection successful
 */
bool attemptWiFiConnection(const char* ssid, const char* password) {
    Serial.printf("Attempting to connect to WiFi network: %s\n", ssid);
    updateStatusIndicator(false, false);
    
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    // Set the hostname before connecting
    WiFi.setHostname(OTA_HOSTNAME);
    WiFi.begin(ssid, password);
    
    unsigned long connectionStartTime = millis();
    
    // Wait for connection or timeout
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - connectionStartTime > WIFI_CONNECT_TIMEOUT) {
            Serial.printf("Failed to connect to %s, timeout reached\n", ssid);
            return false;
        }
        
        delay(500);
        Serial.print(".");
    }
    
    // Connection successful
    Serial.println();
    Serial.printf("Connected to WiFi network: %s\n", ssid);
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal strength (RSSI): %d dBm\n", WiFi.RSSI());
    
    return true;
}

/**
 * @brief Reads WiFi credentials from config file in SPIFFS
 * 
 * @param ssid Buffer to store the SSID
 * @param password Buffer to store the password
 * @return True if credentials were successfully read from file
 */
bool readWiFiCredentials(char* ssid, char* password) {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return false;
    }
    
    File configFile = SPIFFS.open(WIFI_CONFIG_FILE, "r");
    
    if (!configFile) {
        Serial.println("Failed to open WiFi config file");
        // List files for debugging
        printSpiffsFiles();
        return false;
    }
    
    // Check for legacy format (SSID on first line, password on second)
    String firstLine = configFile.readStringUntil('\n');
    firstLine.trim();
    
    if (firstLine.indexOf(':') == -1) {
        // Legacy format detected
        String secondLine = configFile.readStringUntil('\n');
        secondLine.trim();
        configFile.close();
        
        if (firstLine.isEmpty() || secondLine.isEmpty()) {
            Serial.println("WiFi config file format is invalid");
            return false;
        }
        
        // Safe copy to buffers
        if (!copyToBuffer(ssid, firstLine, 32) || 
            !copyToBuffer(password, secondLine, 64)) {
            return false;
        }
        
        // Log success
        logCredentials(ssid, password);
        return true;
    }
    
    // New format with credentials on each line as SSID:PASSWORD
    // Reset file position
    configFile.seek(0);
    
    // Read first entry
    String line = configFile.readStringUntil('\n');
    configFile.close();
    
    line.trim();
    if (line.isEmpty()) {
        Serial.println("WiFi config file is empty");
        return false;
    }
    
    // Extract SSID and password
    int delimiterPos = line.indexOf(':');
    if (delimiterPos == -1) {
        Serial.println("Invalid format in WiFi config file (expected SSID:PASSWORD)");
        return false;
    }
    
    String ssidString = line.substring(0, delimiterPos);
    String passwordString = line.substring(delimiterPos + 1);
    
    // Safe copy to buffers
    if (!copyToBuffer(ssid, ssidString, 32) || 
        !copyToBuffer(password, passwordString, 64)) {
        return false;
    }
    
    // Log success
    logCredentials(ssid, password);
    return true;
}

/**
 * @brief Connects to WiFi network
 * 
 * @return True if connection was successful
 */
bool connectToWiFi() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        updateStatusIndicator(false, false);
        return false;
    }
    
    File configFile = SPIFFS.open(WIFI_CONFIG_FILE, "r");
    
    if (!configFile) {
        Serial.println("Failed to open WiFi config file");
        // List files for debugging
        printSpiffsFiles();
        updateStatusIndicator(false, false);
        return false;
    }
    
    bool connected = false;
    bool legacyFormat = false;
    char ssid[32];
    char password[64];
    
    // Check first line to detect format
    String firstLine = configFile.readStringUntil('\n');
    firstLine.trim();
    
    if (firstLine.indexOf(':') == -1) {
        // Legacy format (SSID on first line, password on second)
        legacyFormat = true;
        String secondLine = configFile.readStringUntil('\n');
        secondLine.trim();
        
        if (!firstLine.isEmpty() && !secondLine.isEmpty()) {
            copyToBuffer(ssid, firstLine, 32);
            copyToBuffer(password, secondLine, 64);
            
            connected = attemptWiFiConnection(ssid, password);
        }
    } else {
        // Reset file position to start
        configFile.seek(0);
        
        // Try each network configuration until successful
        while (!connected && configFile.available()) {
            String line = configFile.readStringUntil('\n');
            line.trim();
            
            if (line.isEmpty()) continue;
            
            int delimiterPos = line.indexOf(':');
            if (delimiterPos == -1) {
                Serial.println("Invalid format in WiFi config (expected SSID:PASSWORD)");
                continue;
            }
            
            String ssidString = line.substring(0, delimiterPos);
            String passwordString = line.substring(delimiterPos + 1);
            
            copyToBuffer(ssid, ssidString, 32);
            copyToBuffer(password, passwordString, 64);
            
            // Log the network we're trying
            Serial.printf("Trying WiFi configuration: %s\n", ssid);
            
            // Attempt connection with this configuration
            connected = attemptWiFiConnection(ssid, password);
        }
    }
    
    configFile.close();
    
    // Update status - WiFi connected (or not), but counter status stays the same
    extern bool isLastRequestSuccessful();
    updateStatusIndicator(connected, isLastRequestSuccessful());
    
    return connected;
}

/**
 * @brief Check WiFi connection and reconnect if needed
 */
void checkAndMaintainWiFi() {
    static bool prevWifiConnected = false;
    bool currentlyConnected = (WiFi.status() == WL_CONNECTED);
    
    // Only take action if connection state has changed
    if (currentlyConnected != prevWifiConnected) {
        prevWifiConnected = currentlyConnected;
        
        if (!currentlyConnected) {
            Serial.println("WiFi connection lost, attempting to reconnect...");
            connectToWiFi();
        } else {
            // WiFi connection was restored
            extern bool isLastRequestSuccessful();
            updateStatusIndicator(true, isLastRequestSuccessful());
        }
    }
}

/**
 * @brief Initialize WiFi connection
 */
void initWiFi() {
    if (!connectToWiFi()) {
        Serial.println("Continuing without WiFi connection");
    }
}

/**
 * @brief Initialize OTA update functionality
 */
void initOTA() {
    // Configure OTA hostname
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    Serial.printf("OTA hostname set to: %s\n", OTA_HOSTNAME);
    
    // Set password for OTA updates
    ArduinoOTA.setPassword(OTA_PASSWORD);
    Serial.printf("OTA password configured (password: %s)\n", OTA_PASSWORD);
    
    // OTA callbacks
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
            // Unmount SPIFFS to avoid data corruption
            SPIFFS.end();
        }
        Serial.println("OTA update started: " + type);
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA update complete");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        unsigned int percentage = (progress / (total / 100));
        Serial.printf("OTA Progress: %u%%\r", percentage);
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });
    
    // Begin OTA service
    ArduinoOTA.begin();
    Serial.println("OTA initialized, ready for update");
}

/**
 * @brief Handle OTA updates in the loop
 */
void handleOTA() {
    ArduinoOTA.handle();
}