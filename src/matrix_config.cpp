#include "matrix_config.h"
#include <SPIFFS.h>
#include <JPEGDecoder.h>

// Global matrix instance
MatrixPanel_I2S_DMA *matrix = nullptr;

/**
 * @brief Update the status indicator in the bottom left pixel
 * @param wifiConnected True if WiFi is connected, false otherwise
 * @param updateSuccessful True if counter update was successful, false if there was an error
 */
void updateStatusIndicator(bool wifiConnected, bool updateSuccessful) {
    if (matrix != nullptr) {
        uint16_t color;
        
        if (!wifiConnected) {
            color = WIFI_DISCONNECTED_COLOR; // Red when WiFi is disconnected
        } else if (!updateSuccessful) {
            color = COUNTER_ERROR_COLOR;     // Orange when WiFi works but update failed
        } else {
            color = WIFI_CONNECTED_COLOR;    // Green when everything works
        }
        
        // Draw a single pixel at bottom left corner (0, PANEL_HEIGHT-1)
        matrix->drawPixel(0, PANEL_HEIGHT-1, color);
    }
}

/**
 * @brief Initialize the LED matrix with the configured settings
 * @return Pointer to the initialized matrix
 */
MatrixPanel_I2S_DMA* initMatrix() {
    // Define pin configuration
    HUB75_I2S_CFG::i2s_pins pins = {R1, G1, BL1, R2, G2, BL2, CH_A, CH_B, CH_C, CH_D, CH_E, LAT, OE, CLK};
    
    // Create matrix configuration
    HUB75_I2S_CFG mxconfig(PANEL_WIDTH, PANEL_HEIGHT, PANELS_NUMBER, pins);
    
    // Additional configuration options
    mxconfig.gpio.e = PIN_E;
    mxconfig.driver = HUB75_I2S_CFG::FM6126A;  // for panels using FM6126A chips
    mxconfig.clkphase = false;                 // Try false to fix pixel bleeding
    
    // Create and initialize matrix
    matrix = new MatrixPanel_I2S_DMA(mxconfig);
    matrix->begin();
    matrix->setBrightness8(255);
    
    // Initialize WiFi status indicator as disconnected by default
    updateStatusIndicator(false, false);
    
    return matrix;
}

/**
 * @brief Calculate RGB565 color from RGB components
 * 
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return RGB565 color value
 */
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    // Convert to RGB565 format (5 bits R, 6 bits G, 5 bits B)
    r = r >> 3;  // 8 bits to 5 bits (0-31)
    g = g >> 2;  // 8 bits to 6 bits (0-63)
    b = b >> 3;  // 8 bits to 5 bits (0-31)
    
    return (r << 11) | (g << 5) | b;
}

// Removed drawDigit function to avoid duplicate definition (now only in counter.cpp)

/**
 * @brief Display a bitmap on the matrix with support for both grayscale and RGB formats
 * 
 * Renders a bitmap to the LED matrix:
 * - For grayscale images (1 channel): Each byte represents a pixel's brightness
 *   with values from 0x00 (background) to 0xFF (foreground)
 * - For RGB images (3 channels): Sequential bytes represent R, G, B values
 * 
 * @param bitmap The bitmap data
 * @param width Width of the bitmap in pixels
 * @param height Height of the bitmap in pixels
 * @param fgColor Foreground color (for brightest pixels in grayscale mode)
 * @param bgColor Background color (for darkest pixels in grayscale mode)
 * @param x X position to start drawing on the matrix (or center X if centerPos is true)
 * @param y Y position to start drawing on the matrix (or center Y if centerPos is true)
 * @param channels Number of channels (1 for grayscale, 3 for RGB)
 * @param centerPos If true, x/y coordinates specify the center of the image instead of top-left
 */
void displayBitmap(const uint8_t* bitmap, uint16_t width, uint16_t height, 
                  uint16_t fgColor, uint16_t bgColor, uint16_t x, uint16_t y,
                  uint8_t channels, bool centerPos) {
    // Calculate the top-left position if center positioning is requested
    uint16_t startX = x;
    uint16_t startY = y;
    
    if (centerPos) {
        // Adjust to make x,y the center of the image
        startX = x - (width / 2);
        startY = y - (height / 2);
    }
    
    // Extract RGB components from foreground and background colors (for grayscale mode)
    uint8_t fg_r = (fgColor >> 11) & 0x1F;
    uint8_t fg_g = (fgColor >> 5) & 0x3F;
    uint8_t fg_b = fgColor & 0x1F;
    
    uint8_t bg_r = (bgColor >> 11) & 0x1F;
    uint8_t bg_g = (bgColor >> 5) & 0x3F;
    uint8_t bg_b = bgColor & 0x1F;
    
    for (uint16_t yy = 0; yy < height; yy++) {
        for (uint16_t xx = 0; xx < width; xx++) {
            uint16_t color;
            
            if (channels == 1) {
                // Grayscale mode (1 byte per pixel)
                uint16_t byteIndex = yy * width + xx;
                
                // Read the grayscale value (0-255)
                uint8_t grayscale = bitmap[byteIndex];
                
                // Calculate the blend ratio (0.0 to 1.0)
                float blend = grayscale / 255.0;
                
                // Interpolate between background and foreground colors
                uint8_t r = bg_r + (fg_r - bg_r) * blend;
                uint8_t g = bg_g + (fg_g - bg_g) * blend;
                uint8_t b = bg_b + (fg_b - bg_b) * blend;
                
                // Convert back to RGB565 format
                color = (r << 11) | (g << 5) | b;
            } 
            else if (channels == 3) {
                // RGB mode (3 bytes per pixel)
                uint16_t byteIndex = (yy * width + xx) * 3;
                
                // Read the RGB values (0-255 for each component)
                uint8_t r = bitmap[byteIndex];      // Red
                uint8_t g = bitmap[byteIndex + 1];  // Green
                uint8_t b = bitmap[byteIndex + 2];  // Blue
                
                // Convert to RGB565 format
                color = rgb565(r, g, b);
            }
            else {
                // Unsupported channel count, display in red to indicate error
                color = 0xF800; // Bright red in RGB565
            }
            
            // Draw the pixel using the calculated start position
            matrix->drawPixel(startX + xx, startY + yy, color);
        }
    }
}

// For backward compatibility, maintain the original function signature
void displayBitmap(const uint8_t* bitmap, uint16_t width, uint16_t height, 
                  uint16_t fgColor, uint16_t bgColor, uint16_t x, uint16_t y,
                  bool centerPos) {
    // Default to grayscale (1 channel)
    displayBitmap(bitmap, width, height, fgColor, bgColor, x, y, 1, centerPos);
}

/**
 * @brief Display a JPEG image from SPIFFS on the matrix
 * @param filename Name of the JPEG file in SPIFFS
 * @param x X position to start drawing (or center X if centerPos is true)
 * @param y Y position to start drawing (or center Y if centerPos is true)
 * @param maxWidth Maximum width to display (will scale if needed)
 * @param maxHeight Maximum height to display (will scale if needed)
 * @param centerPos If true, x/y coordinates specify the center of the image instead of top-left
 * @return True if successful, false if failed
 */
bool displayJPEG(const char* filename, uint16_t x, uint16_t y, uint16_t maxWidth, uint16_t maxHeight, bool centerPos) {
    // Check if SPIFFS is initialized
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed");
        return false;
    }
    
    // Check if file exists
    if (!SPIFFS.exists(filename)) {
        Serial.print("File not found: ");
        Serial.println(filename);
        return false;
    }

    // Open the file
    File jpegFile = SPIFFS.open(filename, "r");
    if (!jpegFile) {
        Serial.print("Failed to open file: ");
        Serial.println(filename);
        return false;
    }

    // Decode JPEG
    JpegDec.decodeSdFile(jpegFile);
    
    // Get image information
    uint16_t jpegWidth = JpegDec.width;
    uint16_t jpegHeight = JpegDec.height;
    
    // Calculate scale factor if maxWidth or maxHeight is specified
    float scaleX = 1.0;
    float scaleY = 1.0;
    
    if (maxWidth > 0 && jpegWidth > maxWidth) {
        scaleX = (float)maxWidth / jpegWidth;
    }
    
    if (maxHeight > 0 && jpegHeight > maxHeight) {
        scaleY = (float)maxHeight / jpegHeight;
    }
    
    // Use smallest scale factor (to fit both dimensions)
    float scale = min(scaleX, scaleY);
    
    // Calculate final dimensions
    uint16_t displayWidth = round(jpegWidth * scale);
    uint16_t displayHeight = round(jpegHeight * scale);
    
    // Calculate the top-left position if center positioning is requested
    uint16_t startX = centerPos ? x - (displayWidth / 2) : x;
    uint16_t startY = centerPos ? y - (displayHeight / 2) : y;
    
    // Output to serial for debugging
    Serial.print("JPEG dimensions: ");
    Serial.print(jpegWidth);
    Serial.print("x");
    Serial.println(jpegHeight);
    Serial.print("Display dimensions: ");
    Serial.print(displayWidth);
    Serial.print("x");
    Serial.println(displayHeight);
    
    // Render the image
    displayJPEGBlocks(startX, startY, scale, displayWidth, displayHeight);
    
    jpegFile.close();
    return true;
}

/**
 * @brief Helper function to display JPEG MCU blocks
 * @param startX Starting X position
 * @param startY Starting Y position
 * @param scale Scale factor
 * @param displayWidth Final display width
 * @param displayHeight Final display height
 */
void displayJPEGBlocks(uint16_t startX, uint16_t startY, float scale, 
                      uint16_t displayWidth, uint16_t displayHeight) {
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    
    // Render each MCU block
    while (JpegDec.read()) {
        // Get the coordinates of the MCU block
        uint16_t mcu_x = JpegDec.MCUx;
        uint16_t mcu_y = JpegDec.MCUy;
        
        // Get the RGB values for each pixel in the MCU
        uint16_t* pImg = JpegDec.pImage;
        
        // Calculate pixel positions in the output image
        for (int py = 0; py < mcu_h; py++) {
            for (int px = 0; px < mcu_w; px++) {
                // Calculate actual position in scaled image
                uint16_t displayX = round((mcu_x * mcu_w + px) * scale) + startX;
                uint16_t displayY = round((mcu_y * mcu_h + py) * scale) + startY;
                
                // Only render if within bounds
                if (displayX < (startX + displayWidth) && displayY < (startY + displayHeight)) {
                    // Get the RGB565 color value
                    uint16_t color = *pImg++;
                    
                    // Draw the pixel
                    matrix->drawPixel(displayX, displayY, color);
                } else {
                    // Skip pixels that won't be displayed
                    pImg++;
                }
            }
        }
    }
}