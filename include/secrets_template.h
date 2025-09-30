#ifndef SECRETS_H
#define SECRETS_H

// ==================== OPENWEATHERMAP CONFIGURATION ====================
// Get your free API key at: https://openweathermap.org/api
#define OPENWEATHERMAP_API_KEY "your_api_key_here"

// API Configuration
#define OPENWEATHERMAP_BASE_URL "https://api.openweathermap.org/data/2.5/weather"
#define OPENWEATHERMAP_CITY "Your_City"              // e.g., "Toronto", "New York", "London"
#define OPENWEATHERMAP_UNITS "metric"                // "metric" for Celsius, "imperial" for Fahrenheit

// Constructed API endpoint (do not modify)
#define OPENWEATHERMAP_API_ENDPOINT OPENWEATHERMAP_BASE_URL "?q=" OPENWEATHERMAP_CITY "&appid=" OPENWEATHERMAP_API_KEY "&units=" OPENWEATHERMAP_UNITS

// ==================== WIFI CONFIGURATION ====================
// Your WiFi network credentials
#define WIFI_SSID "Your_WiFi_Network_Name"
#define WIFI_PASSWORD "Your_WiFi_Password"

// ==================== SETUP INSTRUCTIONS ====================
// 1. Copy this file to secrets.h: cp secrets_template.h secrets.h
// 2. Replace the placeholder values above with your actual credentials
// 3. Add secrets.h to your .gitignore file to keep credentials private
// 4. Build and upload your project
//
// 📖 For detailed setup instructions, see: SECURITY_SETUP.md

#endif // SECRETS_H
