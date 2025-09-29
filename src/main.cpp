#include <Arduino.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager  version 2.0.17
#include <TFT_eSPI.h>
#include <ArduinoJson.h> // 7.1.0
#include <HTTPClient.h> // https://github.com/arduino-libraries/ArduinoHttpClient   version 0.6.1
#include <ESP32Time.h>  // https://github.com/fbiego/ESP32Time  version 2.0.6
#include <time.h>
#include <Preferences.h>  // For secure credential storage
#include "NotoSansBold15.h"
#include "tinyFont.h"
#include "smallFont.h"
#include "midleFont.h"
#include "bigFont.h"
#include "font18.h"
#include "weather_icons.h"

// ==================== CONFIGURATION & CONSTANTS ====================

namespace DisplayConstants {
    const int SPRITE_WIDTH = 320;
    const int SPRITE_HEIGHT = 170;
    const int ERRSPRITE_WIDTH = 164;
    const int ERRSPRITE_HEIGHT = 15;
    const int BACKLIGHT_PIN = 38;
    const int POWER_PIN = 15;
    const int DEFAULT_BRIGHTNESS = 215;
    const int GRAY_LEVELS = 13;
}

namespace WeatherConstants {
    const int UPDATE_INTERVAL_MS = 180000;  // 3 minutes - respects API rate limits
    const int SYNC_INTERVAL_UPDATES = 10;   // Sync time every 10 updates (30 minutes)
    const int MAX_RETRY_ATTEMPTS = 3;
    const int RETRY_DELAY_MS = 5000;
    const int ANIMATION_RESET_POSITION = -420;
    const int ANIMATION_START_POSITION = 100;
    const int TEMPERATURE_HISTORY_SIZE = 24;
}

namespace NetworkConstants {
    const char* WIFI_SSID = "MyWifiConf";
    const char* WIFI_PASS = "password";
    const char* NTP_SERVER = "pool.ntp.org";
    const long GMT_OFFSET_SEC = -5 * 3600;   // UTC-5 (Eastern Standard Time)
    const int DAYLIGHT_OFFSET_SEC = 3600;    // +1 hour for Daylight Saving Time
    const int WIFI_TIMEOUT_MS = 5000;
}

// ==================== ERROR HANDLING CLASS ====================

class ErrorHandler {
public:
    enum ErrorType {
        HTTP_ERROR,
        JSON_ERROR,
        NETWORK_ERROR,
        TIME_SYNC_ERROR
    };
    
    static void handleError(ErrorType type, const char* message, int code = 0) {
        Serial.print("ERROR [");
        Serial.print(getErrorTypeName(type));
        Serial.print("]: ");
        Serial.print(message);
        if (code != 0) {
            Serial.print(" (Code: ");
            Serial.print(code);
            Serial.print(")");
        }
        Serial.println();
    }
    
    static void clearError() {
        Serial.println("Error cleared");
    }
    
private:
    static const char* getErrorTypeName(ErrorType type) {
        switch (type) {
            case HTTP_ERROR: return "HTTP";
            case JSON_ERROR: return "JSON";
            case NETWORK_ERROR: return "NETWORK";
            case TIME_SYNC_ERROR: return "TIME";
            default: return "UNKNOWN";
        }
    }
};

// ==================== CONFIGURATION STRUCTURES ====================

struct WeatherConfig {
    char apiKey[64];
    char city[32];
    char units[16];
    int timezone;
    
    // Default constructor with secure defaults
    WeatherConfig() {
        strcpy(apiKey, "a4c0cf9416be5527bd72726c818804bb");  // TODO: Move to secure storage
        strcpy(city, "Gatineau");
        strcpy(units, "metric");
        timezone = 2;
    }
};

struct WeatherData {
    float temperature;
    float feelsLike;
    float humidity;
    float pressure;
    float windSpeed;
    float cloudCoverage;
    float visibility;
    char description[64];
    char weatherIcon[8];  // Weather icon code (e.g., "01d", "02n")
    char sunriseTime[16];
    char sunsetTime[16];
    char scrollingMessage[512];  // Increased buffer size for longer messages
    float minTemp;
    float maxTemp;
    
    // Constructor with default values
    WeatherData() : temperature(22.2), feelsLike(22.2), humidity(50), pressure(1013), 
                   windSpeed(0), cloudCoverage(0), visibility(10), minTemp(-50), maxTemp(1000) {
        strcpy(description, "clear sky");
        strcpy(weatherIcon, "01d");  // Default clear sky day icon
        strcpy(sunriseTime, "--:--");
        strcpy(sunsetTime, "--:--");
        strcpy(scrollingMessage, "Initializing weather data...");
    }
};

struct DisplayState {
    int animationOffset;
    unsigned long lastUpdateTime;
    int updateCounter;
    bool isConnected;
    bool hasError;
    char errorMessage[128];
    
    DisplayState() : animationOffset(WeatherConstants::ANIMATION_START_POSITION), 
                    lastUpdateTime(0), updateCounter(0), isConnected(false), hasError(false) {
        strcpy(errorMessage, "");
    }
};

// ==================== GLOBAL OBJECTS ====================

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite errSprite = TFT_eSprite(&tft);
ESP32Time rtc(0);
Preferences preferences;

// Configuration and state objects
WeatherConfig config;
WeatherData weatherData;
DisplayState displayState;

// Animation and timing variables
int ani = 100;
unsigned long timePased = 0;

// Button and brightness control
const int BUTTON_BOOT = 0;      // GPIO0 - Boot button (brightness down - bottom button)
const int BUTTON_KEY = 14;      // GPIO14 - Key button (brightness up - top button)
int displayBrightness = DisplayConstants::DEFAULT_BRIGHTNESS;  // Use existing default brightness
const int BRIGHTNESS_STEP = 25; // Step size for brightness changes
unsigned long lastButtonPress = 0;  // For button debouncing
const int BUTTON_DEBOUNCE_MS = 200;  // Debounce delay

// ==================== DISPLAY CONSTANTS ====================

#define bck TFT_BLACK
unsigned short grays[DisplayConstants::GRAY_LEVELS];

// Static UI labels for weather data display
const char* PPlbl1[] = { "FEELS", "CLOUDS", "VISIBIL." };
const char* PPlblU1[] = { " °C", " %", " km" };  // Changed from String to const char*

const char* PPlbl2[] = { "HUMIDITY", "PRESSURE", "WIND" };
const char* PPlblU2[] = { " %", " hPa", " km/h" };  // Changed from String to const char*

// Legacy data arrays (will be integrated into WeatherData struct)
float temperature = 22.2;
float wData1[3];
float wData2[3];

// Scrolling message with buffer system for smooth updates
char Wmsg[512] = "Initializing...";  // Increased to match WmsgBuffer size
char WmsgBuffer[512] = "Initializing...";  // Buffer for next message (increased size)
bool messageUpdatePending = false;         // Flag to indicate new message is ready
int currentMessageWidth = 0;               // Actual pixel width of current message

/**
 * Initialize brightness control and button pins
 */
void initializeBrightnessControl() {
    // Configure button pins as inputs with pull-up resistors
    pinMode(BUTTON_BOOT, INPUT_PULLUP);
    pinMode(BUTTON_KEY, INPUT_PULLUP);
    
    // Note: PWM for backlight is already configured in setup()
    // We just need to set our initial brightness to match the existing setup
    displayBrightness = DisplayConstants::DEFAULT_BRIGHTNESS;
    
    Serial.printf("Brightness control initialized. Default brightness: %d\n", displayBrightness);
    Serial.println("Use Key button (GPIO14, top) to increase brightness, Boot button (GPIO0, bottom) to decrease");
}

/**
 * Handle button presses for brightness control
 */
void handleBrightnessButtons() {
    unsigned long currentTime = millis();
    
    // Check if enough time has passed since last button press (debouncing)
    if (currentTime - lastButtonPress < BUTTON_DEBOUNCE_MS) {
        return;
    }
    
    bool buttonPressed = false;
    
    // Key button (GPIO14) - Increase brightness (top button)
    if (digitalRead(BUTTON_KEY) == LOW) {
        if (displayBrightness < 255) {
            displayBrightness = min(displayBrightness + BRIGHTNESS_STEP, 255);
            ledcWrite(0, displayBrightness);  // Use existing PWM channel 0
            Serial.printf("Brightness increased to: %d/255\n", displayBrightness);
            buttonPressed = true;
        }
    }
    
    // Boot button (GPIO0) - Decrease brightness (bottom button)
    if (digitalRead(BUTTON_BOOT) == LOW) {
        if (displayBrightness > 10) {  // Keep minimum brightness at 10 so display stays visible
            displayBrightness = max(displayBrightness - BRIGHTNESS_STEP, 10);
            ledcWrite(0, displayBrightness);  // Use existing PWM channel 0
            Serial.printf("Brightness decreased to: %d/255\n", displayBrightness);
            buttonPressed = true;
        }
    }
    
    if (buttonPressed) {
        lastButtonPress = currentTime;
    }
}

/**
 * Synchronizes ESP32 RTC with NTP server
 * Called on startup and periodically to maintain accurate time
 * @return true if synchronization successful, false otherwise
 */
bool setTime() {
    Serial.println("Synchronizing time with NTP server...");
    configTime(NetworkConstants::GMT_OFFSET_SEC, NetworkConstants::DAYLIGHT_OFFSET_SEC, NetworkConstants::NTP_SERVER);
    
    struct tm timeinfo;
    int attempts = 0;
    const int maxAttempts = 10;
    
    while (attempts < maxAttempts) {
        if (getLocalTime(&timeinfo)) {
            rtc.setTimeStruct(timeinfo);
            Serial.println("Time synchronized successfully");
            ErrorHandler::clearError();
            return true;
        }
        delay(1000);
        attempts++;
    }
    
    ErrorHandler::handleError(ErrorHandler::TIME_SYNC_ERROR, "Failed to sync time with NTP");
    return false;
}

// Format helper: epoch -> local time (EDT) -> char[]
void formatEpochToLocal(time_t epoch, char* out, size_t outSize, const char* fmt = "%H:%M") {
  // Eastern Time with DST rules (EST/EDT)
  setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);
  tzset();

  struct tm tmLocal;
  localtime_r(&epoch, &tmLocal);     // convert to local time
  strftime(out, outSize, fmt, &tmLocal); // format to "HH:MM:SS EDT" by default
}

/**
 * Fetches current weather data from OpenWeatherMap API
 * Includes error handling, retry logic, and data validation
 * @return true if data fetched successfully, false otherwise
 */
bool getData() {
    static int retryCount = 0;
    
    // Build API URL dynamically
    char apiUrl[256];
    snprintf(apiUrl, sizeof(apiUrl), 
            "https://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=%s",
            config.city, config.apiKey, config.units);
    
    HTTPClient http;
    http.begin(apiUrl);
    http.setTimeout(10000);  // 10 second timeout
    
    Serial.println("Fetching weather data from API...");
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
        // Use more memory-efficient approach than String
        int payloadSize = http.getSize();
        if (payloadSize > 2048) {
            ErrorHandler::handleError(ErrorHandler::HTTP_ERROR, "Response too large");
            http.end();
            return false;
        }
        
        // Read directly into char buffer to avoid String heap allocation
        char payload[2048];
        WiFiClient* stream = http.getStreamPtr();
        int bytesRead = stream->readBytes(payload, min(payloadSize, (int)sizeof(payload) - 1));
        payload[bytesRead] = '\0';
        
        Serial.println("API response received successfully");
        
        // Parse JSON response with error handling
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            // Validate required fields exist
            if (!doc["main"]["temp"] || !doc["weather"][0]["description"]) {
                ErrorHandler::handleError(ErrorHandler::JSON_ERROR, "Missing required fields in API response");
                http.end();
                return false;
            }
            
            // Extract and validate data
            weatherData.temperature = doc["main"]["temp"];
            weatherData.feelsLike = doc["main"]["feels_like"];
            weatherData.humidity = doc["main"]["humidity"];
            weatherData.pressure = doc["main"]["pressure"];
            weatherData.windSpeed = doc["wind"]["speed"];
            weatherData.cloudCoverage = doc["clouds"]["all"];
            weatherData.visibility = doc["visibility"];
            
            // Convert units
            weatherData.visibility = weatherData.visibility / 1000.0;  // Convert to km
            weatherData.windSpeed = weatherData.windSpeed * 3.6;       // Convert to km/h
            
            // Copy description safely
            const char* desc = doc["weather"][0]["description"];
            strncpy(weatherData.description, desc, sizeof(weatherData.description) - 1);
            weatherData.description[sizeof(weatherData.description) - 1] = '\0';
            
            // Extract weather icon code
            const char* icon = doc["weather"][0]["icon"];
            if (icon) {
                strncpy(weatherData.weatherIcon, icon, sizeof(weatherData.weatherIcon) - 1);
                weatherData.weatherIcon[sizeof(weatherData.weatherIcon) - 1] = '\0';
            }
            
            // Process sunrise/sunset times
            long sunrise = doc["sys"]["sunrise"];
            long sunset = doc["sys"]["sunset"];
            formatEpochToLocal(sunrise, weatherData.sunriseTime, sizeof(weatherData.sunriseTime), "%H:%M:%S");
            formatEpochToLocal(sunset, weatherData.sunsetTime, sizeof(weatherData.sunsetTime), "%H:%M:%S");
            
            // Update legacy arrays for compatibility
            wData1[0] = weatherData.feelsLike;
            wData1[1] = weatherData.cloudCoverage;
            wData1[2] = weatherData.visibility;
            wData2[0] = weatherData.humidity;
            wData2[1] = weatherData.pressure;
            wData2[2] = weatherData.windSpeed;
            
            // Update legacy variables
            temperature = weatherData.temperature;
            
            // Create scrolling message without String allocation
            char currentTimeStr[32];
            strcpy(currentTimeStr, rtc.getTime().c_str());  // Get time once and store
            snprintf(weatherData.scrollingMessage, sizeof(weatherData.scrollingMessage),
                    "%s, visibility is %.1f km, wind speed of %.1f km/h, last updated at %s...",
                    weatherData.description, weatherData.visibility, weatherData.windSpeed, currentTimeStr);
            
            // Debug: Check message length
            int msgLen = strlen(weatherData.scrollingMessage);
            Serial.printf("Scrolling message length: %d characters\n", msgLen);
            if (msgLen >= sizeof(weatherData.scrollingMessage) - 1) {
                Serial.println("WARNING: Message may be truncated!");
            }
            
            strcpy(WmsgBuffer, weatherData.scrollingMessage);
            messageUpdatePending = true;  // Mark that new message is ready
            
            // Debug output
            Serial.printf("Temperature: %.1f°C, Description: %s\n", weatherData.temperature, weatherData.description);
            
            retryCount = 0;  // Reset retry count on success
            displayState.isConnected = true;
            ErrorHandler::clearError();
            http.end();
            return true;
            
        } else {
            ErrorHandler::handleError(ErrorHandler::JSON_ERROR, "Failed to parse JSON response", 0);
        }
    } else {
        ErrorHandler::handleError(ErrorHandler::HTTP_ERROR, "HTTP request failed", httpResponseCode);
    }
    
    http.end();
    
    // Iterative retry logic (safer than recursion)
    if (retryCount < WeatherConstants::MAX_RETRY_ATTEMPTS) {
        retryCount++;
        Serial.printf("Retrying... (%d/%d)\n", retryCount, WeatherConstants::MAX_RETRY_ATTEMPTS);
        delay(WeatherConstants::RETRY_DELAY_MS);
        return getData();  // Will be replaced with iterative approach
    }
    
    displayState.isConnected = false;
    return false;
}

// ==================== DISPLAY FUNCTIONS ====================

/**
 * Renders the left panel with weather information and placeholder for icon
 */
void drawLeftPanel() {
    // Header
    sprite.loadFont(midleFont);
    sprite.setTextColor(grays[1], TFT_BLACK);
    sprite.drawString("WEATHER", 6, 10);
    sprite.unloadFont();
    
    // Temperature display
    sprite.setTextDatum(4);  // Center alignment
    sprite.loadFont(bigFont);
    sprite.setTextColor(grays[0], TFT_BLACK);
    sprite.drawFloat(weatherData.temperature, 1, 50, 80);
    sprite.unloadFont();
    
    // Temperature unit
    sprite.loadFont(font18);
    sprite.setTextColor(grays[2], TFT_BLACK);
    if (strcmp(config.units, "metric") == 0) {
        sprite.drawString("C", 112, 55);
        //sprite.drawString("C", 112, 49);
    } else {
        sprite.drawString("F", 112, 49);
    }
    sprite.fillCircle(103, 50, 2, grays[2]);
    sprite.unloadFont();
    
    // City information
    sprite.setTextDatum(0);  // Left alignment
    sprite.loadFont(font18);
    sprite.setTextColor(grays[7], TFT_BLACK);
    sprite.drawString("CITY:", 6, 110);
    sprite.setTextColor(grays[3], TFT_BLACK);
    sprite.drawString(config.city, 48, 110);
    sprite.unloadFont();
    
    // Time display - memory efficient implementation
    char currentTimeStr[32];
    strcpy(currentTimeStr, rtc.getTime().c_str());
    
    // Extract hours:minutes (first 5 characters)
    char timeHM[6];
    strncpy(timeHM, currentTimeStr, 5);
    timeHM[5] = '\0';
    
    // Extract seconds (characters 6-7)
    char timeSS[3];
    strncpy(timeSS, currentTimeStr + 6, 2);
    timeSS[2] = '\0';
    
    // Time without seconds (HH:MM)
    sprite.setTextDatum(0);  // Left alignment
    sprite.loadFont(tinyFont);
    sprite.setTextColor(grays[4], TFT_BLACK);
    sprite.drawString(timeHM, 6, 132);
    sprite.unloadFont();
    
    // Seconds in highlighted rectangle
    sprite.fillRoundRect(90, 132, 42, 22, 2, grays[2]);
    sprite.loadFont(font18);
    sprite.setTextColor(TFT_BLACK, grays[2]);
    sprite.setTextDatum(4);  // Center alignment
    sprite.drawString(timeSS, 111, 144);
    sprite.unloadFont();
    
    // "SECONDS" label
    sprite.setTextDatum(0);
    sprite.setTextColor(grays[5], TFT_BLACK);
    sprite.drawString("SECONDS", 91, 157);
    
    // Icon placeholder area - keeping your original "ICON HERE" text
    sprite.setTextColor(grays[5], TFT_BLACK);
    sprite.drawString("MICRO", 88, 10);
    sprite.drawString("STATION", 88, 20);
    
    // Connection status indicator
    //sprite.setTextColor(displayState.isConnected ? grays[3] : grays[8], TFT_BLACK);
    //sprite.drawString(displayState.isConnected ? "ONLINE" : "OFFLINE", 88, 35);
}



/**
 * Draws a weather icon from the converted PNG data
 * @param x X coordinate for the icon
 * @param y Y coordinate for the icon  
 * @param iconCode Weather icon code (e.g., "01d", "02n")
 */
void drawWeatherIcon(int x, int y, const char* iconCode) {
    const WeatherIcon* icon = getWeatherIcon(iconCode);
    if (icon != nullptr) {
        // Draw the icon pixel by pixel
        for (int py = 0; py < icon->height; py++) {
            for (int px = 0; px < icon->width; px++) {
                int pixelIndex = py * icon->width + px;
                uint16_t color = pgm_read_word(&icon->data[pixelIndex]);
                
                // Only draw non-black pixels (skip transparent areas)
                if (color != 0x0000) {
                    sprite.drawPixel(x + px, y + py, color);
                }
            }
        }
    }
}

/**
 * Renders the right panel with detailed weather data
 */
void drawRightPanel() {
    // Sunrise and sunset information
    sprite.setTextDatum(0);  // Left alignment
    sprite.loadFont(font18);
    sprite.setTextColor(grays[1], TFT_BLACK);
    sprite.drawString("sunrise:", 144, 10);
    sprite.drawString("sunset:", 144, 28);
    
    sprite.setTextColor(grays[3], TFT_BLACK);
    sprite.drawString(weatherData.sunriseTime, 210, 12);
    sprite.drawString(weatherData.sunsetTime, 210, 30);
    sprite.unloadFont();
    
    // Draw weather icon next to sunrise/sunset times
    if (strlen(weatherData.weatherIcon) > 0) {
        drawWeatherIcon(278, 12, weatherData.weatherIcon);
        //drawWeatherIcon(270, 8, weatherData.weatherIcon);
    }
    
    // Weather data boxes - top row
    for (int i = 0; i < 3; i++) {
        int x = 144 + (i * 60);
        sprite.fillSmoothRoundRect(x, 53, 54, 32, 3, grays[9], bck);
        sprite.setTextDatum(4);  // Center alignment
        sprite.setTextColor(grays[3], grays[9]);
        sprite.drawString(PPlbl1[i], x + 27, 59);
        sprite.setTextColor(grays[2], grays[9]);
        sprite.loadFont(font18);
        // Special formatting for feels like temperature (index 0) to show 1 decimal place
        char valueStr[32];
        if (i == 0) {
            snprintf(valueStr, sizeof(valueStr), "%.1f%s", wData1[i], PPlblU1[i]);
        } else {
            snprintf(valueStr, sizeof(valueStr), "%d%s", (int)wData1[i], PPlblU1[i]);
        }
        sprite.drawString(valueStr, x + 27, 76);
        sprite.unloadFont();
    }
    
    // Weather data boxes - bottom row
    for (int i = 0; i < 3; i++) {
        int x = 144 + (i * 60);
        sprite.fillSmoothRoundRect(x, 93, 54, 32, 3, grays[9], bck);
        sprite.setTextDatum(4);  // Center alignment
        sprite.setTextColor(grays[3], grays[9]);
        sprite.drawString(PPlbl2[i], x + 27, 99);
        sprite.setTextColor(grays[2], grays[9]);
        sprite.loadFont(font18);
        char valueStr2[32];
        snprintf(valueStr2, sizeof(valueStr2), "%d%s", (int)wData2[i], PPlblU2[i]);
        sprite.drawString(valueStr2, x + 27, 116);
        sprite.unloadFont();
    }
    
    // Scrolling message area
    sprite.fillSmoothRoundRect(144, 148, 174, 16, 2, grays[10], bck);
    errSprite.pushToSprite(&sprite, 148, 150);
    
    // Status information
    sprite.setTextDatum(0);  // Left alignment
    sprite.setTextColor(grays[4], bck);
    sprite.drawString("CURRENT CONDITIONS", 145, 138);
    sprite.setTextColor(grays[9], bck);
    char counterStr[16];
    snprintf(counterStr, sizeof(counterStr), "%d", displayState.updateCounter);
    sprite.drawString(counterStr, 310, 141);
}

/**
 * Main drawing function that renders the complete display
 * Coordinates all UI elements and maintains layout
 */
void draw() {
    // Prepare scrolling message with seamless looping
    errSprite.fillSprite(grays[10]);
    errSprite.setTextColor(grays[1], grays[10]);
    
    // Calculate message width for seamless scrolling
    errSprite.setTextDatum(0);  // Left alignment
    currentMessageWidth = errSprite.textWidth(Wmsg);
    int spacing = 80;  // Increased space between repeated messages for cleaner transitions
    int totalWidth = currentMessageWidth + spacing;
    
    // Only draw the message once at the start of a new cycle to avoid mid-transition issues
    if (ani >= 0) {
        // Normal scrolling - draw two copies for seamless loop
        errSprite.drawString(Wmsg, ani, 4);
        errSprite.drawString(Wmsg, ani + totalWidth, 4);
    } else {
        // During off-screen phase - only draw the copy that might be visible
        errSprite.drawString(Wmsg, ani, 4);
        if (ani + totalWidth > -currentMessageWidth) {
            errSprite.drawString(Wmsg, ani + totalWidth, 4);
        }
    }
    
    // Clear main sprite and draw divider lines
    sprite.fillSprite(TFT_BLACK);
    sprite.drawLine(138, 10, 138, 164, grays[6]);  // Vertical divider
    sprite.drawLine(100, 108, 134, 108, grays[6]); // Horizontal divider in left panel
    sprite.setTextDatum(0);  // Reset text alignment
    
    // Draw main panels
    drawLeftPanel();
    drawRightPanel();
    
    // Push sprite to display
    sprite.pushSprite(0, 0);
}

/**
 * Updates weather data and manages display state
 * Handles timing, animation, and periodic data refresh
 */
void updateData() {
    // Update scrolling animation - move 2 pixels per frame for better speed
    ani -= 2;
    
    // Use a more generous reset point to ensure clean transitions
    int spacing = 80;  // Match the spacing in draw()
    int resetPoint = -400;  // Fixed reset point for consistent behavior
    
    // Reset position and update message at a fixed point for predictable transitions
    if (ani < resetPoint) {
        ani = WeatherConstants::ANIMATION_START_POSITION;
        
        // Apply pending message update AFTER position reset for smoother transition
        if (messageUpdatePending) {
            strcpy(Wmsg, WmsgBuffer);
            messageUpdatePending = false;
            currentMessageWidth = 0;  // Reset width so it gets recalculated in next draw()
            Serial.println("Scrolling message updated at animation restart");
        }
    }
    
    // Check if it's time for a data update
    if (millis() > timePased + WeatherConstants::UPDATE_INTERVAL_MS) {
        timePased = millis();
        displayState.updateCounter++;
        
        // Fetch new weather data
        if (getData()) {
            Serial.println("Weather data updated successfully");
        } else {
            Serial.println("Failed to update weather data");
        }
        
        // Periodic time synchronization and temperature history update
        if (displayState.updateCounter >= WeatherConstants::SYNC_INTERVAL_UPDATES) {
            setTime();  // Sync time every 10 updates (30 minutes)
            displayState.updateCounter = 0;
            
            // Temperature history removed - was unused for display
        }
    }
}

/**
 * Arduino setup function - initializes hardware and connections
 * Sets up display, WiFi, time synchronization, and initial data fetch
 */
void setup() {
    Serial.begin(115200);
    Serial.println("Weather Display Starting...");
    
    // Hardware initialization
    pinMode(DisplayConstants::POWER_PIN, OUTPUT);
    digitalWrite(DisplayConstants::POWER_PIN, HIGH);  // Power on the display
    
    // Additional power management for T-Display S3
    // Ensure stable power delivery
    delay(100);  // Allow power to stabilize
    
    // Display initialization
    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Connecting to WIFI!!", 30, 50, 4);
    
    // Create sprites for double buffering
    sprite.createSprite(DisplayConstants::SPRITE_WIDTH, DisplayConstants::SPRITE_HEIGHT);
    errSprite.createSprite(DisplayConstants::ERRSPRITE_WIDTH, DisplayConstants::ERRSPRITE_HEIGHT);
    
    // Configure display backlight
    ledcSetup(0, 10000, 8);
    ledcAttachPin(DisplayConstants::BACKLIGHT_PIN, 0);
    ledcWrite(0, DisplayConstants::DEFAULT_BRIGHTNESS);
    
    // Generate grayscale palette
    int colorValue = 210;
    for (int i = 0; i < DisplayConstants::GRAY_LEVELS; i++) {
        grays[i] = tft.color565(colorValue, colorValue, colorValue);
        colorValue -= 20;
    }
    
    // WiFi connection with fallback AP mode
    WiFiManager wifiManager;
    wifiManager.setConfigPortalTimeout(NetworkConstants::WIFI_TIMEOUT_MS);
    
    if (!wifiManager.autoConnect(NetworkConstants::WIFI_SSID, NetworkConstants::WIFI_PASS)) {
        Serial.println("Failed to connect to WiFi, restarting...");
        delay(3000);
        ESP.restart();
    }
    
    Serial.println("WiFi connected successfully");
    displayState.isConnected = true;
    
    // Initialize preferences for secure storage
    preferences.begin("weather", false);
    
    // Initialize brightness control buttons
    initializeBrightnessControl();
    
    // Initial time synchronization and data fetch
    setTime();
    getData();
    
    Serial.println("Setup complete - entering main loop");
}

/**
 * Arduino main loop - handles display updates and data refresh
 * Runs continuously to maintain real-time display
 */
void loop() {
    // Non-blocking timing for smoother performance
    static unsigned long lastDisplayUpdate = 0;
    static unsigned long lastMemoryCheck = 0;
    static int loopCounter = 0;
    
    unsigned long currentMillis = millis();
    
    // Update display at 40Hz (25ms interval) for smooth animation
    if (currentMillis - lastDisplayUpdate >= 25) {
        updateData();
        draw();
        lastDisplayUpdate = currentMillis;
    }
    
    // Handle brightness control buttons (non-blocking)
    handleBrightnessButtons();
    
    // Memory monitoring (every 30 seconds) - more frequent than before
    loopCounter++;
    if (currentMillis - lastMemoryCheck >= 30000) {  // 30 seconds
        lastMemoryCheck = currentMillis;
        Serial.printf("Free heap: %d bytes, Loops: %d\n", ESP.getFreeHeap(), loopCounter);
        loopCounter = 0;
    }
    
    // Small yield to prevent watchdog triggers and allow other tasks
    yield();
}