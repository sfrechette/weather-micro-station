#ifndef WEATHER_DISPLAY_H
#define WEATHER_DISPLAY_H

#include <TFT_eSPI.h>
#include "config.h"
#include "weather_data.h"
#include "weather_icons.h"
#include "NotoSansBold15.h"
#include "tinyFont.h"
#include "smallFont.h"
#include "midleFont.h"
#include "bigFont.h"
#include "font18.h"

// Forward declarations
class ESP32Time;

class WeatherDisplay {
public:
    WeatherDisplay(ESP32Time& rtcRef); // Constructor takes ESP32Time reference
    
    // Initialize display and sprites
    void begin();
    
    // Main drawing functions
    void draw();
    void drawLeftPanel();
    void drawRightPanel();
    void drawWeatherIcon(int x, int y, const char* iconCode);
    
    // Animation and scrolling
    void updateData();
    void updateScrollingMessage();
    
    // Brightness control
    void initializeBrightnessControl();
    void handleBrightnessButtons();
    
    // Getters for external access
    WeatherData& getWeatherData() { return weatherData; }
    DisplayState& getDisplayState() { return displayState; }
    
    // Update scrolling message buffer immediately
    void updateScrollingBuffer();
    WeatherConfig& getConfig() { return config; }
    
    // Legacy data access (for compatibility)
    float& getTemperature() { return temperature; }
    float* getWData1() { return wData1; }
    float* getWData2() { return wData2; }
    char* getWmsg() { return Wmsg; }
    char* getWmsgBuffer() { return WmsgBuffer; }
    bool& getMessageUpdatePending() { return messageUpdatePending; }
    int& getCurrentMessageWidth() { return currentMessageWidth; }
    int& getAni() { return ani; }
    unsigned long& getTimePased() { return timePased; }
    
private:
    // Display objects
    TFT_eSPI tft;
    TFT_eSprite sprite;
    TFT_eSprite errSprite;
    ESP32Time& rtc; // Reference to the global ESP32Time object
    
    // Data structures
    WeatherConfig config;
    WeatherData weatherData;
    DisplayState displayState;
    
    // Animation and timing variables
    int ani;
    unsigned long timePased;
    
    // Button and brightness control
    int displayBrightness;
    unsigned long lastButtonPress;
    
    // Legacy data arrays (for compatibility)
    float temperature;
    float wData1[3];
    float wData2[3];
    
    // Scrolling message with buffer system
    char Wmsg[512];
    char WmsgBuffer[512];
    bool messageUpdatePending;
    int currentMessageWidth;
    bool messageWidthCached;
    
    // Grayscale palette
    unsigned short grays[GRAY_LEVELS];
    
    // UI labels
    const char* PPlbl1[3];
    const char* PPlblU1[3];
    const char* PPlbl2[3];
    const char* PPlblU2[3];
    
    // Helper functions
    void generateGrayscalePalette();
    void setupUILabels();
    void updateLegacyArrays();
    
    // Performance optimization: Font management
    void loadFontOnce(const uint8_t* font);
    void unloadFontOnce();
    const uint8_t* currentFont;
    
    // Performance optimization: Static buffers
    static char timeBuffer[32];
    static char valueStrBuffer[32];
    static char counterStrBuffer[16];
    
    // Performance monitoring
    static unsigned long frameCount;
    static unsigned long lastPerformanceReport;
    static unsigned long lastFrameTime;
    static void reportPerformanceStats();
    
public:
    // Public method for updating legacy arrays
    void updateLegacyData();
};

#endif // WEATHER_DISPLAY_H
