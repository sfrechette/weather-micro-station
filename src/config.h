#ifndef CONFIG_H
#define CONFIG_H

// ==================== DISPLAY CONFIGURATION ====================
#define SPRITE_WIDTH 320
#define SPRITE_HEIGHT 170
#define ERRSPRITE_WIDTH 164
#define ERRSPRITE_HEIGHT 15
#define BACKLIGHT_PIN 38
#define POWER_PIN 15
#define DEFAULT_BRIGHTNESS 215
#define GRAY_LEVELS 13

// ==================== WEATHER CONFIGURATION ====================
#define UPDATE_INTERVAL_MS 180000  // 3 minutes - respects API rate limits
#define SYNC_INTERVAL_UPDATES 10   // Sync time every 10 updates (30 minutes)
#define MAX_RETRY_ATTEMPTS 3
#define RETRY_DELAY_MS 5000
#define ANIMATION_RESET_POSITION -420
#define ANIMATION_START_POSITION 100
#define TEMPERATURE_HISTORY_SIZE 24

// ==================== NETWORK CONFIGURATION ====================
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC -5 * 3600   // UTC-5 (Eastern Standard Time)
#define DAYLIGHT_OFFSET_SEC 3600  // +1 hour for Daylight Saving Time
#define WIFI_TIMEOUT_MS 5000

// ==================== BUTTON CONFIGURATION ====================
#define BUTTON_BOOT 0      // GPIO0 - Boot button (brightness down - bottom button)
#define BUTTON_KEY 14      // GPIO14 - Key button (brightness up - top button)
#define BRIGHTNESS_STEP 25 // Step size for brightness changes
#define BUTTON_DEBOUNCE_MS 200  // Debounce delay

// ==================== DISPLAY COLORS ====================
#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF

// ==================== UI LABELS ====================
// Static UI labels for weather data display
#define PPlbl1_0 "FEELS"
#define PPlbl1_1 "CLOUDS" 
#define PPlbl1_2 "VISIBIL."
#define PPlblU1_0 " °C"
#define PPlblU1_1 " %"
#define PPlblU1_2 " km"

#define PPlbl2_0 "HUMIDITY"
#define PPlbl2_1 "PRESSURE"
#define PPlbl2_2 "WIND"
#define PPlblU2_0 " %"
#define PPlblU2_1 " hPa"
#define PPlblU2_2 " km/h"

#endif // CONFIG_H
