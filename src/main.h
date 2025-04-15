#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <WiFi.h>

// Serial communication settings
#define BAUD_RATE 115200

// WiFi settings
#define WIFI_CONFIG_FILE "/wifi_config.txt"    // Path to WiFi config file in SPIFFS
#define WIFI_CONNECT_TIMEOUT 10000             // WiFi connection timeout in milliseconds

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
 * @brief Setup function called once at startup
 */
void setup();

/**
 * @brief Main program loop
 */
void loop();

#endif // MAIN_H