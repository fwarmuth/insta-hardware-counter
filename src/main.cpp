#include "main.h"
#include "matrix_config.h"
#include "counter.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include "instagram_logo.h"
#include <WiFi.h>

/**
 * @brief Reads WiFi credentials from config file in SPIFFS
 * 
 * @param ssid Buffer to store the SSID
 * @param password Buffer to store the password
 * @return True if credentials were successfully read from file
 */
bool readWiFiCredentials(char* ssid, char* password) {
    // Check if SPIFFS is mounted
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return false;
    }
    
    // Try to open the config file
    File configFile = SPIFFS.open(WIFI_CONFIG_FILE, "r");
    
    if (!configFile) {
        Serial.println("Failed to open WiFi config file");
        // Print all files in SPIFFS root directory for debugging
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        Serial.println("Files in SPIFFS:");
        while(file) {
            Serial.print("  ");
            Serial.print(file.name());
            Serial.print(" (");
            Serial.print(file.size());
            Serial.println(" bytes)");
            file = root.openNextFile();
        }
        return false;
    }
    
    // Read SSID from first line
    String ssidString = configFile.readStringUntil('\n');
    ssidString.trim();
    
    // Read password from second line
    String passwordString = configFile.readStringUntil('\n');
    passwordString.trim();
    
    configFile.close();
    
    // Check if both values were read
    if (ssidString.length() == 0 || passwordString.length() == 0) {
        Serial.println("WiFi config file format is invalid");
        return false;
    }
    
    // Make sure we don't overflow the buffers
    size_t ssidLen = ssidString.length();
    size_t pwdLen = passwordString.length();
    
    // Copy with caution to avoid buffer overflow
    memset(ssid, 0, 32);  // Clear buffer first
    memset(password, 0, 64);  // Clear buffer first
    
    // Copy only what will fit in the buffer (leaving room for null terminator)
    if (ssidLen >= 32) ssidLen = 31;
    if (pwdLen >= 64) pwdLen = 63;
    
    memcpy(ssid, ssidString.c_str(), ssidLen);
    memcpy(password, passwordString.c_str(), pwdLen);
    
    // Ensure null termination
    ssid[ssidLen] = '\0';
    password[pwdLen] = '\0';
    
    // Debug output
    Serial.println("WiFi credentials loaded from config file");
    Serial.print("SSID: [");
    Serial.print(ssid);
    Serial.println("]");
    Serial.print("SSID length: ");
    Serial.println(ssidLen);
    
    Serial.print("Password: [");
    Serial.print(password);
    Serial.println("]");
    Serial.print("Password length: ");
    Serial.println(pwdLen);
    
    // Print hexdump of SSID for debugging encoding issues
    Serial.println("SSID hex values:");
    for (size_t i = 0; i < ssidLen; i++) {
        Serial.print("0x");
        if ((uint8_t)ssid[i] < 0x10) Serial.print("0");
        Serial.print((uint8_t)ssid[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    return true;
}

/**
 * @brief Connects to WiFi network
 * 
 * @return True if connection was successful
 */
bool connectToWiFi() {
    // Buffers to store credentials
    char ssid[32] = "";
    char password[64] = "";
    
    // Try to read credentials from file
    if (!readWiFiCredentials(ssid, password)) {
        Serial.println("Failed to read WiFi credentials from file");
        updateWiFiStatusIndicator(false);
        return false;
    }
    
    Serial.printf("Connecting to WiFi network: %s\n", ssid);
    
    // Update indicator to show disconnected status
    updateWiFiStatusIndicator(false);
    
    // Set WiFi mode to station (client)
    WiFi.mode(WIFI_STA);
    
    // Start WiFi connection
    WiFi.begin(ssid, password);
    
    // Set timeout for connection
    unsigned long connectionStartTime = millis();
    
    // Wait for connection or timeout
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - connectionStartTime > WIFI_CONNECT_TIMEOUT) {
            Serial.println("WiFi connection failed, timeout reached");
            updateWiFiStatusIndicator(false);
            return false;
        }
        
        delay(500);
        Serial.print(".");
    }
    
    // Print connection details
    Serial.println();
    Serial.println("WiFi connected successfully");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    // Update indicator to show connected status
    updateWiFiStatusIndicator(true);
    
    return true;
}

/**
 * @brief Setup function called once at startup
 * 
 * Initializes serial communication, matrix display and counter
 */
void setup() {
    // Initialize serial communication
    Serial.begin(BAUD_RATE);
    Serial.println("Starting counter application...");
    
    // Initialize SPIFFS for loading image files
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed!");
    } else {
        Serial.println("SPIFFS initialized successfully.");
    }
    
    // Initialize the LED matrix first so we can use the status indicator
    initMatrix();
    
    // Connect to WiFi - status indicator will be updated in connectToWiFi()
    if (!connectToWiFi()) {
        Serial.println("Continuing without WiFi connection");
    }
    
    // Initialize the counter
    initCounter();
    
    Serial.println("Initialization complete.");
}

/**
 * @brief Main program loop
 * 
 * Checks if counter needs updating and refreshes display when it does
 */

u_long loopCounter = 0;
void loop() {
    loopCounter++;
    // Take start time
    unsigned long startMillis = millis();
    // Check WiFi connection and reconnect if needed
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost, attempting to reconnect...");
        // Status indicator will be updated in connectToWiFi()
        connectToWiFi();
    } else {
        // Ensure status indicator shows connected
        updateWiFiStatusIndicator(true);
    }
    
    // Check if counter needs updating
    bool counterUpdated = updateCounter();
    
    if (counterUpdated) {
        Serial.println("Counter updated");
    }
    
    // Clear the matrix display
    matrix->clearScreen();
    
    // Always update the counter display
    displayCounter();
    
    // Make sure WiFi status indicator is still visible after display refresh
    updateWiFiStatusIndicator(WiFi.status() == WL_CONNECTED);
    
    // Rate limit the loop to 250ms, but only if we haven't already exceeded that time
    unsigned long elapsedTime = millis() - startMillis;
    if (elapsedTime < 50) {
        delay(50 - elapsedTime);
    }
    else {
        Serial.println("Loop took longer than 50ms, skipping delay");
    }
    if (loopCounter % 1000 == 0) {
        Serial.println("Loop counter: " + String(loopCounter));
        Serial.println("Loop took: " + String(millis() - startMillis) + "ms");
    }
}
