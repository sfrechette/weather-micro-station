#include "weather_api.h"
#include <ESP32Time.h> // Include ESP32Time for rtc object

// ErrorHandler implementation (moved from main.cpp)
void ErrorHandler::handleError(ErrorType type, const char* message, int code) {
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

void ErrorHandler::clearError() {
    Serial.println("Error cleared");
}

const char* ErrorHandler::getErrorTypeName(ErrorType type) {
    switch (type) {
        case HTTP_ERROR: return "HTTP";
        case JSON_ERROR: return "JSON";
        case NETWORK_ERROR: return "NETWORK";
        case TIME_SYNC_ERROR: return "TIME";
        default: return "UNKNOWN";
    }
}

WeatherAPI::WeatherAPI(ESP32Time& rtcRef) : rtc(rtcRef) {
    // Constructor initialization
}

// connectWiFi() removed - WiFi connection now handled in main.cpp

bool WeatherAPI::setTime() {
    Serial.println("Synchronizing time with NTP server...");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    
    struct tm timeinfo;
    int attempts = 0;
    const int maxAttempts = 10;
    
    while (attempts < maxAttempts) {
        if (getLocalTime(&timeinfo)) {
            Serial.println("Time synchronized successfully");
            return true;
        }
        delay(1000);
        attempts++;
    }
    
    Serial.println("Failed to sync time with NTP");
    return false;
}

bool WeatherAPI::getData(WeatherData& weatherData, DisplayState& displayState) {
    Serial.printf("=== FETCHING WEATHER DATA [%lu ms] ===\n", millis());
    Serial.printf("API URL: %s\n", OPENWEATHERMAP_API_ENDPOINT);
    
    // Use full API endpoint from secrets.h
    const char* apiUrl = OPENWEATHERMAP_API_ENDPOINT;
    
    HTTPClient http;
    http.begin(apiUrl);
    http.setTimeout(10000);  // 10 second timeout
    
    Serial.println("Fetching weather data from API...");
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
        // Use more memory-efficient approach than String
        int payloadSize = http.getSize();
        if (payloadSize > 2048) {
            Serial.println("Response too large");
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
                Serial.println("Missing required fields in API response");
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
            
            // Set last updated to current local time when API fetch happened
            time_t now = time(nullptr);
            struct tm* timeinfo = localtime(&now);
            if (timeinfo != nullptr) {
                strftime(weatherData.lastUpdated, sizeof(weatherData.lastUpdated), "%H:%M:%S", timeinfo);
            } else {
                strcpy(weatherData.lastUpdated, "12:00:00");
            }
            
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
            
            // Simple API data output
            Serial.println("API VALUES:");
            Serial.printf("Temp: %.1f°C | Feels: %.1f°C | Humidity: %.0f%% | Pressure: %.0f hPa\n", 
                         weatherData.temperature, weatherData.feelsLike, weatherData.humidity, weatherData.pressure);
            Serial.printf("Wind: %.1f km/h | Clouds: %.0f%% | Visibility: %.1f km | %s\n", 
                         weatherData.windSpeed, weatherData.cloudCoverage, weatherData.visibility, weatherData.description);
            Serial.printf("Updated: %s\n", weatherData.lastUpdated);
            Serial.println("=== API FETCH SUCCESS ===");
            
            displayState.isConnected = true;
            http.end();
            return true;
            
        } else {
            Serial.println("ERROR: Failed to parse JSON response");
        }
    } else {
        Serial.printf("ERROR: HTTP request failed with code: %d\n", httpResponseCode);
    }
    
    http.end();
    displayState.isConnected = false;
    Serial.println("=== API FETCH FAILED ===");
    return false;
}

void WeatherAPI::formatEpochToLocal(time_t epoch, char* out, size_t outSize, const char* fmt) {
    // Eastern Time with DST rules (EST/EDT)
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);
    tzset();

    struct tm tmLocal;
    localtime_r(&epoch, &tmLocal);     // convert to local time
    strftime(out, outSize, fmt, &tmLocal); // format to "HH:MM:SS EDT" by default
}

