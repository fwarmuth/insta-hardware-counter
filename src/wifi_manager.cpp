#include "wifi_manager.h"

// Global variables for captive portal functionality
WebServer webServer(WEB_SERVER_PORT);
DNSServer dnsServer;
bool captivePortalActive = false;
unsigned long portalStartTime = 0;

// Forward declarations of captive portal handlers
void handleRoot();
void handleSave();
void handleNotFound();

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

/**
 * @brief Writes new WiFi credentials to the config file
 */
bool writeWiFiCredentials(const char* ssid, const char* password) {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return false;
    }
    
    File configFile = SPIFFS.open(WIFI_CONFIG_FILE, "w");
    if (!configFile) {
        Serial.println("Failed to open WiFi config file for writing");
        return false;
    }
    
    // Write in the format SSID:PASSWORD
    configFile.printf("%s:%s\n", ssid, password);
    configFile.close();
    
    Serial.println("WiFi credentials written to config file");
    return true;
}

/**
 * @brief Start the captive portal for WiFi configuration
 */
void startCaptivePortal() {
    // Stop any existing WiFi connection
    WiFi.disconnect();
    delay(100);
    
    // Set up access point
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(AP_IP_ADDRESS), IPAddress(AP_IP_ADDRESS), IPAddress(255, 255, 255, 0));
    
    // Start the access point with SSID and password
    if (WiFi.softAP(AP_SSID, AP_PASSWORD)) {
        Serial.println("Access Point started");
        Serial.printf("SSID: %s\n", AP_SSID);
        Serial.printf("Password: %s\n", AP_PASSWORD);
        Serial.printf("AP IP address: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("Failed to start Access Point");
        return;
    }
    
    // Start DNS server for captive portal
    dnsServer.start(DNS_PORT, "*", IPAddress(AP_IP_ADDRESS));
    
    // Set up web server routes
    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/save", HTTP_POST, handleSave);
    webServer.onNotFound(handleNotFound);
    
    // Start web server
    webServer.begin();
    Serial.println("Captive portal started");
    
    // Set the start time for timeout tracking
    portalStartTime = millis();
    captivePortalActive = true;

    // Visual indicator that we're in AP mode
    updateStatusIndicator(false, false);
}

/**
 * @brief Handle captive portal in the main loop
 * @return True if portal is still active, false if it was closed
 */
bool handleCaptivePortal() {
    if (!captivePortalActive) {
        return false;
    }
    
    // Check for portal timeout
    if (millis() - portalStartTime > PORTAL_TIMEOUT_MS) {
        Serial.println("Captive portal timeout reached");
        captivePortalActive = false;
        
        // Try to connect with any existing credentials
        if (connectToWiFi()) {
            Serial.println("Connected to WiFi after portal timeout");
        } else {
            Serial.println("No WiFi connection after portal timeout");
        }
        
        return false;
    }
    
    // Process DNS and web server requests
    dnsServer.processNextRequest();
    webServer.handleClient();
    
    return true;
}

/**
 * @brief Handle root page of captive portal
 */
void handleRoot() {
    // Read existing WiFi credentials if available
    char ssid[32] = "";
    char password[64] = "";
    bool hasExistingConfig = readWiFiCredentials(ssid, password);
    
    // Create the configuration web page
    String html = "<!DOCTYPE html><html><head>"
                  "<title>ESP WiFi Setup</title>"
                  "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                  "<style>"
                  "body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#f5f5f5;color:#333;line-height:1.6;}"
                  "h1{color:#0066cc;text-align:center;margin-bottom:30px;}"
                  ".container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}"
                  ".form-group{margin-bottom:15px;}"
                  "label{display:block;margin-bottom:5px;font-weight:bold;}"
                  "input[type=text],input[type=password]{width:100%;padding:10px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box;}"
                  "button{background:#0066cc;color:white;border:none;padding:12px;width:100%;border-radius:4px;cursor:pointer;font-size:16px;}"
                  "button:hover{background:#0055aa;}"
                  ".footer{text-align:center;margin-top:20px;font-size:12px;color:#666;}"
                  "</style>"
                  "</head><body>"
                  "<div class='container'>"
                  "<h1>Instagram Counter WiFi Setup</h1>"
                  "<form method='post' action='/save'>"
                  "<div class='form-group'>"
                  "<label for='ssid'>WiFi Network Name (SSID):</label>"
                  "<input type='text' id='ssid' name='ssid' value='";
    
    // Add existing SSID if available
    html += hasExistingConfig ? ssid : "";
    
    html += "' required>"
            "</div>"
            "<div class='form-group'>"
            "<label for='password'>WiFi Password:</label>"
            "<input type='password' id='password' name='password' value='";
    
    // Add existing password if available
    html += hasExistingConfig ? password : "";
    
    html += "' required>"
            "</div>"
            "<button type='submit'>Save Configuration</button>"
            "</form>"
            "<div class='footer'>After saving, the device will attempt to connect to your WiFi network.</div>"
            "</div>"
            "</body></html>";
    
    webServer.send(200, "text/html", html);
}

/**
 * @brief Handle form submission with new WiFi credentials
 */
void handleSave() {
    String newSsid = webServer.arg("ssid");
    String newPassword = webServer.arg("password");
    
    // Validate inputs
    if (newSsid.length() == 0) {
        webServer.send(400, "text/plain", "SSID cannot be empty");
        return;
    }
    
    Serial.println("Received new WiFi credentials:");
    Serial.printf("SSID: %s\n", newSsid.c_str());
    Serial.println("Password: [hidden]");
    
    // Convert String to char arrays
    char ssidBuffer[32] = {0};
    char passwordBuffer[64] = {0};
    
    copyToBuffer(ssidBuffer, newSsid, 32);
    copyToBuffer(passwordBuffer, newPassword, 64);
    
    // Save to config file
    bool saved = writeWiFiCredentials(ssidBuffer, passwordBuffer);
    
    // Send response
    String html = "<!DOCTYPE html><html><head>"
                  "<title>WiFi Configuration</title>"
                  "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                  "<style>"
                  "body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#f5f5f5;color:#333;line-height:1.6;}"
                  ".container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1);text-align:center;}"
                  "h1{color:";
    
    // Change title color based on result
    html += saved ? "#4CAF50" : "#f44336";
    
    html += ";}"
            "</style>"
            "</head><body>"
            "<div class='container'>"
            "<h1>";
            
    if (saved) {
        html += "Configuration Saved!";
    } else {
        html += "Error Saving Configuration";
    }
    
    html += "</h1>"
            "<p>";
            
    if (saved) {
        html += "WiFi credentials have been saved. The device will now attempt to connect to your network.";
    } else {
        html += "There was a problem saving your WiFi credentials. Please try again.";
    }
    
    html += "</p>"
            "</div>"
            "</body></html>";
    
    webServer.send(200, "text/html", html);
    
    // If saved successfully, try to connect
    if (saved) {
        // Wait a bit to make sure the client gets the response
        delay(2000);
        
        // Stop captive portal
        captivePortalActive = false;
        webServer.stop();
        WiFi.softAPdisconnect(true);
        dnsServer.stop();
        
        // Try to connect with new credentials
        if (attemptWiFiConnection(ssidBuffer, passwordBuffer)) {
            Serial.println("Successfully connected with new credentials");
        } else {
            Serial.println("Failed to connect with new credentials");
            // Restart captive portal if connection fails
            startCaptivePortal();
        }
    }
}

/**
 * @brief Handle all undefined URLs in the captive portal
 * 
 * This is needed to redirect all requests to the configuration page
 */
void handleNotFound() {
    // For captive portal, redirect all requests to the root
    webServer.sendHeader("Location", "/", true);
    webServer.send(302, "text/plain", "");
}

/**
 * @brief Initialize WiFi with fallback to captive portal
 */
void initWiFiWithCaptivePortal() {
    // First try to connect with saved credentials
    if (connectToWiFi()) {
        Serial.println("Connected to WiFi with saved credentials");
        initOTA();
    } else {
        // If connection fails, start the captive portal
        Serial.println("WiFi connection failed. Starting captive portal.");
        startCaptivePortal();
    }
}