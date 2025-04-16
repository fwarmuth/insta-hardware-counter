#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoOTA.h>  // Include the OTA library
#include "matrix_config.h" // For access to the updateStatusIndicator function

// WiFi settings
#define WIFI_CONFIG_FILE "/wifi_config.txt"    // Path to WiFi config file in SPIFFS
#define WIFI_CONNECT_TIMEOUT 10000             // WiFi connection timeout in milliseconds

// OTA settings
#define OTA_HOSTNAME "insta_counter"
#define OTA_PASSWORD "your_ota_password"       // Make sure this matches platformio.ini upload_flags auth

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

#endif // WIFI_MANAGER_H