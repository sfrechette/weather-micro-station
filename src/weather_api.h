#ifndef WEATHER_API_H
#define WEATHER_API_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <WiFiManager.h>
#include "config.h"
#include "weather_data.h"
#include "secrets.h"

// Forward declarations
class ESP32Time; // Forward declare ESP32Time

class ErrorHandler {
public:
    enum ErrorType {
        HTTP_ERROR,
        JSON_ERROR,
        NETWORK_ERROR,
        TIME_SYNC_ERROR
    };

    static void handleError(ErrorType type, const char* message, int code = 0);
    static void clearError();

private:
    static const char* getErrorTypeName(ErrorType type);
};

class WeatherAPI {
public:
    WeatherAPI(ESP32Time& rtcRef); // Constructor takes ESP32Time reference

    bool connectWiFi(); // No longer needed as WiFiManager is in main
    bool setTime();
    bool getData(WeatherData& weatherData, DisplayState& displayState);

private:
    ESP32Time& rtc; // Reference to the global ESP32Time object

    // Helper functions
    void formatEpochToLocal(time_t epoch, char* out, size_t outSize, const char* fmt = "%H:%M");
};

#endif // WEATHER_API_H
