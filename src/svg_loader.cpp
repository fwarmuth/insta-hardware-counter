#include "svg_loader.h"
#include "counter.h"
#include <SPIFFS.h>
#include <regex>

// Buffer for loading SVG file
static char svgBuffer[8192]; // Adjust size based on your needs and ESP32 memory
static uint8_t tempIconData[72]; // Temporary buffer for 24x24 icon (72 bytes)

// Forward declarations for local functions
void drawLine(bool grid[][24], int gridSize, float x0, float y0, float x1, float y1, 
              int svgWidth, int svgHeight);
void floodFill(bool grid[][24], int gridSize, int x, int y);

/**
 * @brief Initialize the filesystem for storing SVG files
 * @return True if initialization was successful
 */
bool initSVGFileSystem() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
        return false;
    }
    Serial.println("SPIFFS mounted successfully");
    return true;
}

/**
 * @brief Simple SVG parser to convert viewBox and path data to a bitmap
 * @param svgContent The SVG file content as a string
 * @param iconData Pointer to array where bitmap data will be stored
 * @param maxSize Maximum size of the iconData array
 * @return True if parsing was successful
 */
bool parseSVG(const char* svgContent, uint8_t* iconData, size_t maxSize) {
    // Clear the icon data buffer first
    memset(iconData, 0, maxSize);
    
    // For simple 24x24 icons, we need a 72-byte buffer (24*24 bits / 8 bits per byte)
    if (maxSize < 72) {
        Serial.println("Icon buffer too small");
        return false;
    }
    
    // Find viewBox to determine SVG dimensions
    char* viewBoxPos = strstr(svgContent, "viewBox");
    if (viewBoxPos == NULL) {
        Serial.println("viewBox not found");
        return false;
    }
    
    // Parse viewBox (format: viewBox="0 0 width height")
    int minX, minY, width, height;
    if (sscanf(viewBoxPos, "viewBox=\"%d %d %d %d\"", &minX, &minY, &width, &height) != 4) {
        Serial.println("Invalid viewBox format");
        return false;
    }
    
    // Check if we have a square SVG (we only support square icons for now)
    if (width != height) {
        Serial.println("Only square SVGs are supported");
        return false;
    }
    
    // Find path data (we're looking for the basic path element)
    char* pathPos = strstr(svgContent, "<path");
    if (pathPos == NULL) {
        Serial.println("Path element not found");
        return false;
    }
    
    // Find d attribute in path
    char* dPos = strstr(pathPos, "d=\"");
    if (dPos == NULL) {
        Serial.println("Path data not found");
        return false;
    }
    
    // Extract path data
    dPos += 3; // Skip 'd="'
    char* dEnd = strchr(dPos, '"');
    if (dEnd == NULL) {
        Serial.println("Path data format invalid");
        return false;
    }
    
    // Create a temporary path data buffer
    size_t pathLen = dEnd - dPos;
    char* pathData = new char[pathLen + 1];
    if (pathData == NULL) {
        Serial.println("Failed to allocate memory for path data");
        return false;
    }
    strncpy(pathData, dPos, pathLen);
    pathData[pathLen] = '\0';
    
    // For simple SVGs, we'll use a basic rasterization approach
    // We'll draw to a 24x24 pixel grid
    const int gridSize = 24;
    bool grid[gridSize][gridSize];
    memset(grid, 0, sizeof(grid));
    
    // Basic path parser - this is a simplified version that handles only basic SVGs
    // It will fill the grid with true for pixels that should be colored
    char* cmd = strtok(pathData, " ,");
    float x = 0, y = 0;
    float startX = 0, startY = 0;
    
    while (cmd != NULL) {
        if (*cmd == 'M' || *cmd == 'm') {
            // Move command
            float newX = atof(strtok(NULL, " ,"));
            float newY = atof(strtok(NULL, " ,"));
            
            if (*cmd == 'm') {
                // Relative coordinates
                x += newX;
                y += newY;
            } else {
                // Absolute coordinates
                x = newX;
                y = newY;
            }
            
            startX = x;
            startY = y;
        } else if (*cmd == 'L' || *cmd == 'l') {
            // Line command
            float newX = atof(strtok(NULL, " ,"));
            float newY = atof(strtok(NULL, " ,"));
            
            if (*cmd == 'l') {
                // Relative coordinates
                newX += x;
                newY += y;
            }
            
            // Draw line from (x,y) to (newX,newY)
            drawLine(grid, gridSize, x, y, newX, newY, width, height);
            
            x = newX;
            y = newY;
        } else if (*cmd == 'H' || *cmd == 'h') {
            // Horizontal line
            float newX = atof(strtok(NULL, " ,"));
            
            if (*cmd == 'h') {
                // Relative coordinates
                newX += x;
            }
            
            // Draw horizontal line
            drawLine(grid, gridSize, x, y, newX, y, width, height);
            
            x = newX;
        } else if (*cmd == 'V' || *cmd == 'v') {
            // Vertical line
            float newY = atof(strtok(NULL, " ,"));
            
            if (*cmd == 'v') {
                // Relative coordinates
                newY += y;
            }
            
            // Draw vertical line
            drawLine(grid, gridSize, x, y, x, newY, width, height);
            
            y = newY;
        } else if (*cmd == 'Z' || *cmd == 'z') {
            // Close path
            drawLine(grid, gridSize, x, y, startX, startY, width, height);
            x = startX;
            y = startY;
        }
        
        cmd = strtok(NULL, " ,");
    }
    
    delete[] pathData;
    
    // Convert grid to bitmap format
    for (int row = 0; row < gridSize; row++) {
        for (int col = 0; col < gridSize; col++) {
            // Calculate which byte and bit this pixel belongs to
            int byteIndex = (row * gridSize + col) / 8;
            int bitIndex = 7 - ((row * gridSize + col) % 8); // MSB first
            
            if (grid[row][col]) {
                // Set the bit if the grid cell is filled
                iconData[byteIndex] |= (1 << bitIndex);
            }
        }
    }
    
    return true;
}

/**
 * @brief Draw a line on a grid using Bresenham's algorithm
 * @param grid The grid to draw on
 * @param gridSize Size of the grid
 * @param x0 Starting x coordinate
 * @param y0 Starting y coordinate
 * @param x1 Ending x coordinate
 * @param y1 Ending y coordinate
 * @param svgWidth Original SVG width
 * @param svgHeight Original SVG height
 */
void drawLine(bool grid[][24], int gridSize, float x0, float y0, float x1, float y1, 
              int svgWidth, int svgHeight) {
    // Scale from SVG coordinates to grid coordinates
    int gridX0 = (int)(x0 * gridSize / svgWidth);
    int gridY0 = (int)(y0 * gridSize / svgHeight);
    int gridX1 = (int)(x1 * gridSize / svgWidth);
    int gridY1 = (int)(y1 * gridSize / svgHeight);
    
    // Clip coordinates to grid boundaries
    gridX0 = constrain(gridX0, 0, gridSize - 1);
    gridY0 = constrain(gridY0, 0, gridSize - 1);
    gridX1 = constrain(gridX1, 0, gridSize - 1);
    gridY1 = constrain(gridY1, 0, gridSize - 1);
    
    // Bresenham's line algorithm
    int dx = abs(gridX1 - gridX0);
    int dy = -abs(gridY1 - gridY0);
    int sx = gridX0 < gridX1 ? 1 : -1;
    int sy = gridY0 < gridY1 ? 1 : -1;
    int err = dx + dy;
    int e2;
    
    while (true) {
        // Set the pixel
        grid[gridY0][gridX0] = true;
        
        if (gridX0 == gridX1 && gridY0 == gridY1) break;
        
        e2 = 2 * err;
        if (e2 >= dy) {
            if (gridX0 == gridX1) break;
            err += dy;
            gridX0 += sx;
        }
        if (e2 <= dx) {
            if (gridY0 == gridY1) break;
            err += dx;
            gridY0 += sy;
        }
    }
}

/**
 * @brief Fill an area on a grid using flood fill algorithm
 * @param grid The grid to fill
 * @param gridSize Size of the grid
 * @param x Starting x coordinate
 * @param y Starting y coordinate
 */
void floodFill(bool grid[][24], int gridSize, int x, int y) {
    if (x < 0 || x >= gridSize || y < 0 || y >= gridSize || grid[y][x]) {
        return;
    }
    
    grid[y][x] = true;
    
    floodFill(grid, gridSize, x + 1, y);
    floodFill(grid, gridSize, x - 1, y);
    floodFill(grid, gridSize, x, y + 1);
    floodFill(grid, gridSize, x, y - 1);
}

/**
 * @brief Load an SVG file from SPIFFS and convert it to a bitmap
 * @param fileName Name of the SVG file in SPIFFS
 * @param iconData Pointer to array where bitmap data will be stored
 * @param maxSize Maximum size of the iconData array
 * @return True if loading and conversion was successful
 */
bool loadSVGFromFile(const char* fileName, uint8_t* iconData, size_t maxSize) {
    // Check if file exists
    if (!SPIFFS.exists(fileName)) {
        Serial.print("File not found: ");
        Serial.println(fileName);
        return false;
    }
    
    // Open the file
    File file = SPIFFS.open(fileName, "r");
    if (!file) {
        Serial.print("Failed to open file: ");
        Serial.println(fileName);
        return false;
    }
    
    // Read file into buffer
    size_t fileSize = file.size();
    if (fileSize >= sizeof(svgBuffer)) {
        Serial.println("SVG file too large for buffer");
        file.close();
        return false;
    }
    
    file.readBytes(svgBuffer, fileSize);
    svgBuffer[fileSize] = '\0'; // Null-terminate the string
    file.close();
    
    // Parse SVG and convert to bitmap
    bool success = parseSVG(svgBuffer, iconData, maxSize);
    
    if (!success) {
        Serial.print("Failed to parse SVG: ");
        Serial.println(fileName);
    }
    
    return success;
}

/**
 * @brief Display an SVG file directly from SPIFFS
 * @param fileName Name of the SVG file in SPIFFS
 * @param primaryColor The primary color for the icon (for '1' bits)
 * @param secondaryColor The secondary color for the icon (for '0' bits)
 * @param x X position to display the icon (top left corner)
 * @param y Y position to display the icon (top left corner)
 * @return True if loading and displaying was successful
 */
bool displaySVGFromFile(const char* fileName, uint16_t primaryColor, uint16_t secondaryColor, int16_t x, int16_t y) {
    // Load the SVG file and convert to bitmap
    if (!loadSVGFromFile(fileName, tempIconData, sizeof(tempIconData))) {
        return false;
    }
    
    // Display the icon using the existing function
    displayIcon(tempIconData, primaryColor, secondaryColor, x, y);
    
    return true;
}