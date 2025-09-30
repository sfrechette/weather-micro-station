# Security Setup Guide - Weather Micro Station

This guide explains how to securely configure your Weather Micro Station project with API keys, WiFi credentials, and other sensitive information.

## Overview

The Weather Micro Station requires several sensitive credentials:

- **OpenWeatherMap API Key** - For fetching weather data
- **WiFi Credentials** - For internet connectivity
- **Optional**: Custom API endpoints and configuration

## Quick Setup

### Step 1: Copy the Template

```bash
cp include/secrets_template.h include/secrets.h
```

### Step 2: Edit Your Credentials

Open `include/secrets.h` and replace the placeholder values:

```c
// Replace with your actual OpenWeatherMap API key
#define OPENWEATHERMAP_API_KEY "your_actual_api_key_here"

// Replace with your WiFi network details
#define WIFI_SSID "Your_Actual_WiFi_Network"
#define WIFI_PASSWORD "Your_Actual_WiFi_Password"

// Replace with your city name
#define OPENWEATHERMAP_CITY "Your_City"
```

### Step 3: Verify Security

- ✅ Ensure `secrets.h` is listed in `.gitignore`
- ✅ Never commit `secrets.h` to version control
- ✅ Keep API keys private and secure

## Getting Your OpenWeatherMap API Key

### Free API Key (Recommended)

1. **Visit**: [OpenWeatherMap API](https://openweathermap.org/api)
2. **Sign Up**: Create a free account
3. **Generate Key**: Go to "My API Keys" section
4. **Copy Key**: Use the generated key in your `secrets.h`

### API Key Features

- ✅ **Free Tier**: 1,000 calls/day, 60 calls/hour
- ✅ **Current Weather**: Real-time weather data
- ✅ **Multiple Cities**: Support for any city worldwide
- ✅ **Rate Limits**: Automatically respected by our 3-minute intervals

## WiFi Configuration

### Supported Networks

- ✅ **WPA2/WPA3**: Secure wireless networks
- ✅ **2.4GHz**: ESP32-S3 compatible frequency
- ❌ **5GHz Only**: Not supported by ESP32-S3
- ❌ **Enterprise**: WPA2-Enterprise not supported

### WiFi Troubleshooting

```c
// For networks with special characters, use escape sequences:
#define WIFI_SSID "Network \"With Quotes\""
#define WIFI_PASSWORD "Password\\With\\Backslashes"

// For networks with spaces:
#define WIFI_SSID "My Home Network"
#define WIFI_PASSWORD "My Complex Password 123!"
```

## City Configuration

### Supported City Formats

```c
// City name only
#define OPENWEATHERMAP_CITY "Toronto"

// City with country code (recommended for accuracy)
#define OPENWEATHERMAP_CITY "Toronto,CA"

// City with state and country
#define OPENWEATHERMAP_CITY "New York,NY,US"

// International cities
#define OPENWEATHERMAP_CITY "London,UK"
#define OPENWEATHERMAP_CITY "Tokyo,JP"
#define OPENWEATHERMAP_CITY "Paris,FR"
```

### City Name Tips

- Use English names for best compatibility
- Include country codes to avoid ambiguity
- Check [OpenWeatherMap City List](https://openweathermap.org/find) for exact names

## Security Best Practices

### File Protection

```bash
# Verify secrets.h is in .gitignore
grep -q "secrets.h" .gitignore && echo "✅ Protected" || echo "❌ Not protected"

# Check file permissions (should be readable only by you)
chmod 600 include/secrets.h
```

### API Key Security

- **Never share** your API key publicly
- **Rotate keys** periodically (every 6 months)
- **Monitor usage** on OpenWeatherMap dashboard
- **Use environment variables** in production

### WiFi Security

- Use **WPA3** or **WPA2** encryption
- Avoid **open networks** for device operation
- Consider **guest networks** for IoT devices
- **Change default** router passwords

## Advanced Configuration

### Custom API Endpoints

```c
// For different OpenWeatherMap plans or custom proxies
#define OPENWEATHERMAP_BASE_URL "https://api.openweathermap.org/data/2.5/weather"

// For different units (metric = Celsius, imperial = Fahrenheit)
#define OPENWEATHERMAP_UNITS "metric"  // or "imperial"
```

### Development vs Production

```c
// Development - shorter intervals for testing
#define UPDATE_INTERVAL_MS 60000  // 1 minute

// Production - respect API rate limits
#define UPDATE_INTERVAL_MS 180000  // 3 minutes
```

## Troubleshooting

### Common API Issues

**401 Unauthorized**

```
ERROR: HTTP request failed with code: 401
```

- ❌ Invalid or missing API key
- ✅ Verify API key in `secrets.h`
- ✅ Check OpenWeatherMap account status

**404 Not Found**

```
ERROR: HTTP request failed with code: 404
```

- ❌ Invalid city name
- ✅ Check city spelling and format
- ✅ Try adding country code

**429 Too Many Requests**

```
ERROR: HTTP request failed with code: 429
```

- ❌ API rate limit exceeded
- ✅ Increase `UPDATE_INTERVAL_MS`
- ✅ Check API usage on OpenWeatherMap dashboard

### WiFi Connection Issues

**Connection Timeout**

```
Failed to connect to WiFi, restarting...
```

- ❌ Wrong SSID or password
- ❌ Network out of range
- ❌ 5GHz network (use 2.4GHz)
- ✅ Verify credentials in `secrets.h`
- ✅ Check network availability

**Frequent Disconnections**

- ❌ Weak signal strength
- ❌ Router issues
- ✅ Move device closer to router
- ✅ Check router stability

## Environment Variables (Advanced)

For production deployments, consider using environment variables:

### PlatformIO Build Flags

```ini
; platformio.ini
[env:production]
build_flags = 
    -DOPENWEATHERMAP_API_KEY=\"${sysenv.WEATHER_API_KEY}\"
    -DWIFI_SSID=\"${sysenv.WIFI_NETWORK}\"
    -DWIFI_PASSWORD=\"${sysenv.WIFI_PASS}\"
```

### System Environment

```bash
# Set environment variables
export WEATHER_API_KEY="your_api_key_here"
export WIFI_NETWORK="your_network_name"
export WIFI_PASS="your_wifi_password"

# Build with environment variables
pio run -e production
```

## Security Checklist

Before deploying your Weather Micro Station:

- [ ] ✅ `secrets.h` created from template
- [ ] ✅ Real API key added (not placeholder)
- [ ] ✅ WiFi credentials verified and tested
- [ ] ✅ City name confirmed on OpenWeatherMap
- [ ] ✅ `secrets.h` added to `.gitignore`
- [ ] ✅ File permissions set appropriately
- [ ] ✅ API usage monitoring enabled
- [ ] ✅ Network security verified (WPA2/WPA3)
- [ ] ✅ Device placed in secure location
- [ ] ✅ Regular security updates planned

## Emergency Procedures

### Compromised API Key

1. **Immediate**: Delete key from OpenWeatherMap dashboard
2. **Generate**: Create new API key
3. **Update**: Replace key in `secrets.h`
4. **Deploy**: Upload new firmware
5. **Monitor**: Check for unauthorized usage

### Lost Access

1. **Reset**: Factory reset device if needed
2. **Reconfigure**: Set up new `secrets.h`
3. **Test**: Verify connectivity before deployment

## Support Resources

- **OpenWeatherMap**: [API Documentation](https://openweathermap.org/api)
- **ESP32**: [WiFi Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- **Project Issues**: [GitHub Issues](https://github.com/your-repo/issues)

---
