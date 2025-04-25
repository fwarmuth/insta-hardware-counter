#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoOTA.h>  // Include the OTA library
#include <WebServer.h>   // For captive portal web server
#include <DNSServer.h>   // For captive DNS server
#include "matrix_config.h" // For access to the updateStatusIndicator function

// WiFi settings
#define WIFI_CONFIG_FILE "/wifi_config.txt"    // Path to WiFi config file in SPIFFS
#define WIFI_CONNECT_TIMEOUT 10000             // WiFi connection timeout in milliseconds

// AP Mode settings
#define AP_SSID "InstagramCounterConfig"             // AP mode SSID
#define AP_PASSWORD "configure123"             // AP mode password (min 8 chars)
#define AP_IP_ADDRESS 192, 168, 4, 1           // AP mode IP address
#define DNS_PORT 53                            // Standard DNS port
#define WEB_SERVER_PORT 80                     // Standard HTTP port
#define PORTAL_TIMEOUT_MS 300000               // 5 minutes timeout for portal mode

// OTA settings
#define OTA_HOSTNAME "insta_counter"
#define OTA_PASSWORD "123456789"       // Make sure this matches platformio.ini upload_flags auth

/**
 * @brief Lists all files in SPIFFS root directory
 */
void printSpiffsFiles();

/**
 * @brief Safely copies string to buffer with size checking
 * 
 * @param dest Destination buffer
 * @param source Source string
 * @param maxSize Maximum buffer size
 * @return True if successful
 */
bool copyToBuffer(char* dest, String source, size_t maxSize);

/**
 * @brief Logs credential information for debugging
 */
void logCredentials(const char* ssid, const char* password);

/**
 * @brief Reads WiFi credentials from config file in SPIFFS
 * @param ssid Buffer to store the SSID
 * @param password Buffer to store the password
 * @return True if credentials were successfully read from file
 */
bool readWiFiCredentials(char* ssid, char* password);

/**
 * @brief Connects to WiFi network
 * @return True if connection was successful
 */
bool connectToWiFi();

/**
 * @brief Check WiFi connection and reconnect if needed
 */
void checkAndMaintainWiFi();

/**
 * @brief Initialize OTA update functionality
 */
void initOTA();

/**
 * @brief Handle OTA updates in the loop
 */
void handleOTA();

/**
 * @brief Initialize WiFi connection
 */
void initWiFi();

/**
 * @brief Set up a captive portal for WiFi configuration
 * 
 * Creates an access point and hosts a simple web server
 * allowing users to configure WiFi credentials
 */
void startCaptivePortal();

/**
 * @brief Process captive portal requests in the main loop
 * @return True if the portal is active, False if it's been closed
 */
bool handleCaptivePortal();

/**
 * @brief Writes new WiFi credentials to the config file
 * @param ssid New SSID to write
 * @param password New password to write
 * @return True if credentials were successfully written
 */
bool writeWiFiCredentials(const char* ssid, const char* password);

/**
 * @brief Initialize WiFi with fallback to captive portal
 * 
 * Tries to connect to WiFi using saved credentials first,
 * and if that fails, starts a captive portal for configuration
 */
void initWiFiWithCaptivePortal();

#endif // WIFI_MANAGER_H