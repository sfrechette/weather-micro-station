/*
 * Weather Micro Station for T-Display S3
 * Author: Stéphane Fréchette
 * Version: 2.0 (Modular Architecture)
 * 
 * Features:
 * - Displays weather data from OpenWeatherMap API
 * - Modular, maintainable code structure
 * - Proper error handling and recovery
 * - Optimized performance and memory usage
 * - Brightness control via buttons
 * - Scrolling weather messages
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Time.h>
#include <Preferences.h>
#include "config.h"
#include "weather_display.h"
#include "weather_api.h"
#include "secrets.h"

// Global objects
ESP32Time rtc(0);
Preferences preferences;
WeatherDisplay display(rtc); // Pass rtc to display
WeatherAPI apiClient(rtc); // Pass rtc to API client

// Animation and timing variables
unsigned long timePased = 0;

/**
 * Arduino setup function - initializes hardware and connections
 * Sets up display, WiFi, time synchronization, and initial data fetch
 */
void setup() {
    Serial.begin(115200);
    Serial.println("Weather Display Starting...");
    
    // Initialize display
    display.begin();
    
    // Direct WiFi connection using credentials from secrets.h
    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to WiFi, restarting...");
        delay(3000);
        ESP.restart();
    }
    
    Serial.println("\nWiFi connected successfully!");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
    display.getDisplayState().isConnected = true;
    
    // Initialize preferences for secure storage
    preferences.begin("weather", false);
    
    display.initializeBrightnessControl();
    
    // Initial time synchronization and data fetch
    apiClient.setTime();
    
    Serial.println("=== STARTUP: Making initial API call ===");
    // Set "Fetching data..." message for startup
    strcpy(display.getWeatherData().scrollingMessage, "... Fetching data ...");
    display.updateScrollingBuffer();
    Serial.println("Scrolling: ... Fetching data ...");
    
    // Make initial API call
    if (apiClient.getData(display.getWeatherData(), display.getDisplayState())) {
        display.updateLegacyData();
        display.updateScrollingMessage();
        display.updateScrollingBuffer();
        Serial.println("=== STARTUP: Initial API call successful ===");
    } else {
        Serial.println("=== STARTUP: Initial API call failed ===");
    }
    
    // Reset timer for 3-minute intervals starting now
    timePased = millis();
    Serial.printf("=== STARTUP: 3-minute timer started at %lu ms ===\n", timePased);
    Serial.printf("=== STARTUP: Next API call in 3 minutes at %lu ms ===\n", timePased + UPDATE_INTERVAL_MS);
    
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
        // Update animation and scrolling
        display.updateData();
        
        // Check if it's time for a data update (every 3 minutes)
        if (millis() > timePased + UPDATE_INTERVAL_MS) {
            timePased = millis();
            display.getDisplayState().updateCounter++;
            
            Serial.printf("=== 3-MINUTE TIMER: Starting API fetch [%lu ms] ===\n", millis());
            
            // Set "Fetching data..." message BEFORE API call
            strcpy(display.getWeatherData().scrollingMessage, "... Fetching data ...");
            display.updateScrollingBuffer();
            Serial.println("Scrolling: ... Fetching data ...");
            
            // Fetch new weather data
            bool apiSuccess = apiClient.getData(display.getWeatherData(), display.getDisplayState());
            
            if (apiSuccess) {
                // Update legacy arrays for compatibility
                display.updateLegacyData();
                
                // Update scrolling message with fetched data
                display.updateScrollingMessage();
                
                // Update display buffer with the actual data
                display.updateScrollingBuffer();
            } else {
                Serial.println("API call failed");
                // Keep "Fetching data..." message on failure
            }
            
            // Periodic time synchronization
            if (display.getDisplayState().updateCounter >= SYNC_INTERVAL_UPDATES) {
                apiClient.setTime();  // Sync time every 10 updates (30 minutes)
                display.getDisplayState().updateCounter = 0;
            }
        }
        
        // Draw the display
        display.draw();
        lastDisplayUpdate = currentMillis;
    }
    
    // Handle brightness control buttons (non-blocking)
    display.handleBrightnessButtons();
    
    // Memory monitoring (every 30 seconds)
    loopCounter++;
    if (currentMillis - lastMemoryCheck >= 30000) {  // 30 seconds
        lastMemoryCheck = currentMillis;
        Serial.printf("Free heap: %d bytes, Loops: %d\n", ESP.getFreeHeap(), loopCounter);
        loopCounter = 0;
    }
    
    // Small yield to prevent watchdog triggers and allow other tasks
    yield();
}