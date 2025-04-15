#ifndef MATRIX_CONFIG_H
#define MATRIX_CONFIG_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Pin definitions for matrix
#define R1 25
#define G1 27
#define BL1 26
#define R2 14
#define G2 13
#define BL2 12
#define CH_A 23
#define CH_B 19
#define CH_C 5
#define CH_D 17
#define CH_E -1  // assign to any available pin if using panels with 1/32 scan
#define CLK 16
#define LAT 4
#define OE 15
#define PIN_E 32

// Matrix dimensions configuration
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 32
#define PANELS_NUMBER 1

// Derived dimensions
#define PANE_WIDTH (PANEL_WIDTH * PANELS_NUMBER)
#define PANE_HEIGHT PANEL_HEIGHT
#define NUM_LEDS (PANE_WIDTH * PANE_HEIGHT)

// WiFi status indicator colors
#define WIFI_CONNECTED_COLOR 0x07E0    // Green in RGB565
#define WIFI_DISCONNECTED_COLOR 0xF800  // Red in RGB565

// Function declarations
/**
 * @brief Initialize the LED matrix
 * @return Pointer to the initialized matrix
 */
MatrixPanel_I2S_DMA* initMatrix();

/**
 * @brief Display a bitmap on the matrix with support for both grayscale and RGB formats
 * @param bitmap The bitmap data
 * @param width Width of the bitmap
 * @param height Height of the bitmap
 * @param fgColor Foreground color (for brightest pixels in grayscale mode)
 * @param bgColor Background color (for darkest pixels in grayscale mode) 
 * @param x X position to start drawing
 * @param y Y position to start drawing
 * @param channels Number of channels (1 for grayscale, 3 for RGB)
 * @param centerPos If true, x/y coordinates specify the center of the image instead of top-left
 */
void displayBitmap(const uint8_t* bitmap, uint16_t width, uint16_t height, 
                  uint16_t fgColor, uint16_t bgColor, uint16_t x, uint16_t y,
                  uint8_t channels, bool centerPos = false);

/**
 * @brief Display a monochrome bitmap on the matrix
 * @param bitmap The bitmap data
 * @param width Width of the bitmap
 * @param height Height of the bitmap
 * @param fgColor Foreground color for 1 bits
 * @param bgColor Background color for 0 bits
 * @param x X position to start drawing
 * @param y Y position to start drawing
 * @param centerPos If true, x/y coordinates specify the center of the image instead of top-left
 */
void displayBitmap(const uint8_t* bitmap, uint16_t width, uint16_t height, 
                  uint16_t fgColor, uint16_t bgColor, uint16_t x, uint16_t y,
                  bool centerPos = false);

/**
 * @brief Display a JPEG image from SPIFFS on the matrix
 * @param filename Name of the JPEG file in SPIFFS
 * @param x X position to start drawing
 * @param y Y position to start drawing
 * @param maxWidth Maximum width to display (will scale if needed)
 * @param maxHeight Maximum height to display (will scale if needed)
 * @param centerPos If true, x/y coordinates specify the center of the image instead of top-left
 * @return True if successful, false if failed
 */
bool displayJPEG(const char* filename, uint16_t x, uint16_t y, uint16_t maxWidth=0, uint16_t maxHeight=0, bool centerPos=false);

/**
 * @brief Helper function to display JPEG MCU blocks
 * @param startX Starting X position
 * @param startY Starting Y position
 * @param scale Scale factor
 * @param displayWidth Final display width
 * @param displayHeight Final display height
 */
void displayJPEGBlocks(uint16_t startX, uint16_t startY, float scale, 
                      uint16_t displayWidth, uint16_t displayHeight);

/**
 * @brief Update the WiFi status indicator in the bottom left pixel
 * @param connected True if WiFi is connected, false otherwise
 */
void updateWiFiStatusIndicator(bool connected);

extern MatrixPanel_I2S_DMA *matrix;

#endif // MATRIX_CONFIG_H