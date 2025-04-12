#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <WiFi.h>

// Serial communication settings
#define BAUD_RATE 115200

// WiFi settings
// #define WIFI_SSID "_DMZ"      // Replace with your WiFi network name
// #define WIFI_PASSWORD "h4ll0h4ll0"   // Replace with your WiFi password
#define WIFI_SSID "Hidden Network"      // Replace with your WiFi network name
#define WIFI_PASSWORD "h4ll0h4ll0"   // Replace with your WiFi password
#define WIFI_CONNECT_TIMEOUT 10000     // WiFi connection timeout in milliseconds

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