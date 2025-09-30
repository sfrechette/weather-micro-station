#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

#include "config.h"
#include "secrets.h"

// ==================== WEATHER CONFIGURATION STRUCTURE ====================
struct WeatherConfig {
    char apiKey[64];
    char city[32];
    char units[16];
    int timezone;
    
    // Default constructor with secure defaults
    WeatherConfig() {
        strcpy(apiKey, OPENWEATHERMAP_API_KEY);  // Using API key from secrets.h
        strcpy(city, OPENWEATHERMAP_CITY);       // Using city from secrets.h
        strcpy(units, OPENWEATHERMAP_UNITS);     // Using units from secrets.h
        timezone = 2;
    }
};

// ==================== WEATHER DATA STRUCTURE ====================
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
    char lastUpdated[32];       // Last updated datetime from API
    float minTemp;
    float maxTemp;
    
    // Constructor with default values
    WeatherData() : temperature(22.2), feelsLike(22.2), humidity(50), pressure(1013), 
                   windSpeed(5.0), cloudCoverage(25), visibility(10), minTemp(-50), maxTemp(1000) {
        strcpy(description, "clear sky");
        strcpy(weatherIcon, "01d");  // Default clear sky day icon
        strcpy(sunriseTime, "--:--");
        strcpy(sunsetTime, "--:--");
        strcpy(scrollingMessage, "Initializing weather data...");
        strcpy(lastUpdated, "12:00:00");
    }
};

// ==================== DISPLAY STATE STRUCTURE ====================
struct DisplayState {
    int animationOffset;
    unsigned long lastUpdateTime;
    int updateCounter;
    bool isConnected;
    bool hasError;
    char errorMessage[128];
    
    DisplayState() : animationOffset(ANIMATION_START_POSITION), 
                    lastUpdateTime(0), updateCounter(0), isConnected(false), hasError(false) {
        strcpy(errorMessage, "");
    }
};

// ==================== ERROR HANDLING ENUM ====================
enum class ErrorType {
    HTTP_ERROR,
    JSON_ERROR,
    NETWORK_ERROR,
    TIME_SYNC_ERROR
};

#endif // WEATHER_DATA_H
