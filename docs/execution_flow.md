# Weather Station Execution Flow Analysis

## Complete Function Call Hierarchy

### Startup Phase (Runs Once - ~10-15 seconds)

```
main.cpp::setup()
├── Serial.begin(115200)                           // Initialize debug output
├── display.begin()                                // Initialize display system
│   ├── TFT_eSPI initialization                   // Hardware display setup
│   ├── TFT_eSprite creation                      // Double buffering sprites  
│   ├── Font loading (initial fonts)             // Load default fonts
│   └── Brightness control setup                 // Button initialization
├── WiFi.begin(WIFI_SSID, WIFI_PASSWORD)         // Direct WiFi connection
│   ├── Connection attempt (30 second timeout)   // Robust connection logic
│   ├── IP address assignment                     // DHCP configuration
│   └── Signal strength check                     // Connection quality
├── preferences.begin("weather", false)           // Persistent storage init
├── display.initializeBrightnessControl()         // Hardware button setup
├── apiClient.setTime()                           // Time synchronization
│   ├── configTime(GMT_OFFSET, DST_OFFSET, NTP)  // NTP server configuration
│   ├── NTP server connection                     // Network time sync
│   └── rtc.setTime() - Set internal clock       // Local RTC update
├── CLEAR ANIMATION: ani = ANIMATION_START_POSITION // Reset scrolling
├── SET MESSAGE: "... Fetching data ..."         // User feedback
├── display.updateScrollingBuffer()               // Immediate display update
├── delay(2000)                                   // 2-second user visibility
├── apiClient.getData()                           // Initial weather fetch
│   ├── HTTPClient.begin(OPENWEATHERMAP_API_ENDPOINT) // HTTP setup
│   ├── http.GET()                               // API request
│   ├── JSON parsing (ArduinoJson library)       // Response processing
│   ├── Extract weather data:                     // Data extraction
│   │   ├── temperature, feelsLike               // Temperature data
│   │   ├── humidity, pressure                   // Atmospheric data  
│   │   ├── windSpeed, cloudCoverage             // Weather conditions
│   │   ├── visibility, description              // Additional info
│   │   └── Set lastUpdated = current local time // Timestamp
│   └── Update WeatherData struct                 // Internal data storage
├── display.updateLegacyData()                    // Copy to display arrays
├── display.updateScrollingMessage()              // Format ticker text
│   └── Format: "... description, visibility is Xkm/h, wind of Ykm/h, last updated at HH:MM:SS ..."
├── RESET ANIMATION: ani = ANIMATION_START_POSITION // Fresh start for data
├── display.updateScrollingBuffer()               // Display new message
└── timePased = millis()                         // Start 3-minute timer
```

### Main Loop (Runs Continuously at 40Hz)

```
main.cpp::loop()
├── Timing Control: Check 25ms passed? (40 FPS display update)
│   ├── display.updateData()                     // Animation update
│   │   ├── Update animation variables (ani, scroll position)
│   │   ├── Update scrolling text position       // Smooth movement
│   │   └── Handle message transitions           // Buffer management
│   ├── API Timer Check: 3 minutes passed?      // Every 180,000ms
│   │   ├── YES: 3-MINUTE API FETCH SEQUENCE     // Weather data refresh
│   │   │   ├── timePased = millis()            // Reset timer
│   │   │   ├── updateCounter++                  // Track API calls
│   │   │   ├── CLEAR: ani = ANIMATION_START_POSITION // Stop current animation
│   │   │   ├── SET: "... Fetching data ..."    // User feedback message
│   │   │   ├── display.updateScrollingBuffer()  // Immediate display
│   │   │   ├── delay(2000)                     // 2-second visibility
│   │   │   ├── apiClient.getData()             // HTTP API call [Same as startup]
│   │   │   ├── IF SUCCESS:                     // Data processing
│   │   │   │   ├── display.updateLegacyData()  // Copy to display arrays
│   │   │   │   ├── display.updateScrollingMessage() // Format new message
│   │   │   │   ├── RESET: ani = ANIMATION_START_POSITION // Fresh animation
│   │   │   │   └── display.updateScrollingBuffer() // Show new data
│   │   │   ├── ELSE: Keep "Fetching data..." message // Error handling
│   │   │   └── Time Sync Check: 10 updates?    // Every 30 minutes
│   │   │       └── apiClient.setTime()          // NTP resynchronization
│   │   └── NO: Continue normal display cycle    // Regular operation
│   └── display.draw()                           // Render everything
│       ├── drawLeftPanel()                      // Left side content
│       │   ├── Draw current time (rtc.getTime()) // HH:MM:SS format
│       │   ├── Draw current date (rtc.getDate()) // DD/MM/YYYY format  
│       │   ├── Draw temperature (large font)    // Main temperature display
│       │   └── Draw "Micro Station" branding    // Project identifier
│       ├── drawRightPanel()                     // Right side content
│       │   ├── drawWeatherIcon()               // Weather condition icon
│       │   ├── Draw weather data boxes:         // Data grid display
│       │   │   ├── Feels like temperature      // Perceived temperature
│       │   │   ├── Humidity percentage         // Air moisture
│       │   │   ├── Pressure (hPa)             // Atmospheric pressure
│       │   │   ├── Wind speed (km/h)          // Wind velocity
│       │   │   ├── Cloud coverage (%)         // Sky conditions
│       │   │   └── Visibility (km)            // Sight distance
│       │   └── Draw scrolling ticker           // Animated message
│       │       ├── Calculate text position     // Smooth scrolling math
│       │       ├── Handle text wrapping        // Seamless loop
│       │       └── Render with proper fonts    // Typography
│       └── sprite.pushSprite(0, 0)            // Double-buffer push to display
├── display.handleBrightnessButtons()            // User input handling
│   ├── Check button states                     // Hardware polling
│   ├── Adjust brightness levels                // Display control
│   └── Save preferences                        // Persistent settings
├── Memory Monitoring: Every 30 seconds         // Performance tracking
│   ├── ESP.getFreeHeap()                      // Available memory
│   ├── Loop counter tracking                   // Performance metrics
│   └── Serial output diagnostics              // Debug information
└── yield()                                     // ESP32 task scheduling
```

## Timing Intervals & Performance

| Operation | Interval | Purpose | Performance Impact |
|-----------|----------|---------|-------------------|
| **Display Update** | 25ms (40 FPS) | Smooth animation | High CPU, smooth UX |
| **API Calls** | 180,000ms (3 min) | Weather data refresh | Network I/O, rate limit compliance |
| **Time Sync** | 10 API calls (30 min) | Clock accuracy | Minimal, background operation |
| **Memory Check** | 30,000ms (30 sec) | Performance monitoring | Negligible, debug only |
| **"Fetching" Display** | 2,000ms (2 sec) | User feedback | UX enhancement |
| **Button Polling** | Every loop cycle | User input | Minimal overhead |

## Key Data Structures

### WeatherData Struct

```cpp
struct WeatherData {
    float temperature;           // Current temperature (°C)
    float feelsLike;            // Perceived temperature (°C)  
    float humidity;             // Humidity percentage (%)
    float pressure;             // Atmospheric pressure (hPa)
    float windSpeed;            // Wind velocity (km/h)
    float cloudCoverage;        // Cloud coverage (%)
    float visibility;           // Visibility distance (km)
    char description[64];       // Weather description text
    char scrollingMessage[512]; // Formatted ticker message
    char lastUpdated[32];       // Last API fetch time (HH:MM:SS)
    // ... additional fields
};
```

### DisplayState Struct  

```cpp
struct DisplayState {
    bool isConnected;           // WiFi connection status
    int updateCounter;          // API call counter for sync timing
    unsigned long lastUpdate;   // Timestamp of last successful update
    int brightness;             // Current display brightness level
    // ... additional state fields
};
```

### WeatherConfig Struct

```cpp
struct WeatherConfig {
    const char* apiKey;         // OpenWeatherMap API key
    const char* city;           // Target city for weather data
    const char* units;          // Temperature units (metric/imperial)
    int updateInterval;         // API call frequency (milliseconds)
    // ... configuration parameters
};
```

## Critical Execution Paths

### 1. Startup Path (Success Flow)

```
Power On → Serial Init → Display Init → WiFi Connect → Time Sync → 
Initial API Call → Data Display → Enter Main Loop
```

**Duration**: ~10-15 seconds  
**Critical Points**: WiFi connection, API availability, time sync

### 2. 3-Minute Timer Path (Data Refresh)

```
Timer Trigger → Animation Reset → Show "Fetching" → API Call → 
Parse Data → Update Display → Reset Animation → Continue Loop
```

**Duration**: ~3-5 seconds  
**Critical Points**: Network connectivity, API response time, JSON parsing

### 3. Display Loop Path (Continuous)

```
Update Animation → Check Timers → Render Display → Handle Input → 
Memory Check → Yield → Repeat
```

**Duration**: 25ms per cycle  
**Critical Points**: Rendering performance, memory usage, smooth animation

### 4. API Flow Path (Network Operations)

```
HTTP Request → Server Response → JSON Parsing → Data Extraction → 
Timestamp Creation → Struct Update → Success/Failure Return
```

**Duration**: 1-3 seconds  
**Critical Points**: Network latency, API rate limits, JSON validity

## Performance Optimizations

### Font Management

- **Load on demand**: Fonts loaded only when needed
- **Unload after use**: Memory freed immediately  
- **Caching strategy**: Frequently used fonts kept in memory

### Message Buffering

- **Width caching**: Text width calculated once per message
- **Static buffers**: Fixed-size arrays reduce memory allocation
- **Double buffering**: Smooth transitions between messages

### Network Efficiency

- **Connection reuse**: HTTP client optimized for multiple calls
- **Timeout handling**: Robust error recovery mechanisms
- **Rate limit compliance**: 3-minute intervals respect API limits

### Memory Management

- **Stack optimization**: Local variables minimized
- **Heap monitoring**: Regular memory usage tracking
- **Buffer reuse**: Static arrays prevent fragmentation

## Debug & Monitoring Points

### Serial Output Markers

```
[TRACE] Function entry/exit tracking
[API]   HTTP request/response logging  
[DISP]  Display update notifications
[PERF]  Performance timing measurements
[ERROR] Error conditions and recovery
```

### Key Monitoring Values

- **Free heap memory**: ESP.getFreeHeap()
- **WiFi signal strength**: WiFi.RSSI()
- **API response time**: HTTP request duration
- **Display frame rate**: Actual FPS measurement
- **Loop execution time**: Performance profiling

This comprehensive execution flow provides complete visibility into your weather station's operation, from startup through continuous operation, enabling effective debugging, optimization, and feature development.
