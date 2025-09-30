/**
 * Function Call Tracing for Weather Station
 * Add this to your includes for runtime call tracing
 */

#ifndef TRACE_FUNCTIONS_H
#define TRACE_FUNCTIONS_H

#include <Arduino.h>

// Enable/disable tracing
#define ENABLE_FUNCTION_TRACING 1

#if ENABLE_FUNCTION_TRACING
    #define TRACE_FUNCTION() Serial.printf("[TRACE] %lu: %s()\n", millis(), __FUNCTION__)
    #define TRACE_FUNCTION_WITH_PARAM(param) Serial.printf("[TRACE] %lu: %s(%s)\n", millis(), __FUNCTION__, param)
    #define TRACE_ENTER(func) Serial.printf("[ENTER] %lu: %s()\n", millis(), func)
    #define TRACE_EXIT(func) Serial.printf("[EXIT]  %lu: %s()\n", millis(), func)
    #define TRACE_API_CALL(url) Serial.printf("[API]   %lu: Calling %s\n", millis(), url)
    #define TRACE_DISPLAY_UPDATE(msg) Serial.printf("[DISP]  %lu: %s\n", millis(), msg)
#else
    #define TRACE_FUNCTION()
    #define TRACE_FUNCTION_WITH_PARAM(param)
    #define TRACE_ENTER(func)
    #define TRACE_EXIT(func)
    #define TRACE_API_CALL(url)
    #define TRACE_DISPLAY_UPDATE(msg)
#endif

// Performance monitoring
class PerformanceMonitor {
private:
    unsigned long startTime;
    const char* functionName;
    
public:
    PerformanceMonitor(const char* name) : functionName(name) {
        startTime = millis();
        Serial.printf("[PERF] START: %s at %lu ms\n", functionName, startTime);
    }
    
    ~PerformanceMonitor() {
        unsigned long duration = millis() - startTime;
        Serial.printf("[PERF] END:   %s took %lu ms\n", functionName, duration);
    }
};

// Macro for easy performance monitoring
#define MONITOR_PERFORMANCE(name) PerformanceMonitor monitor(name)

// Call stack depth tracking
extern int callStackDepth;

class CallStackTracker {
private:
    const char* functionName;
    
public:
    CallStackTracker(const char* name) : functionName(name) {
        for (int i = 0; i < callStackDepth; i++) Serial.print("  ");
        Serial.printf("→ %s()\n", functionName);
        callStackDepth++;
    }
    
    ~CallStackTracker() {
        callStackDepth--;
        for (int i = 0; i < callStackDepth; i++) Serial.print("  ");
        Serial.printf("← %s()\n", functionName);
    }
};

#define TRACK_CALL_STACK() CallStackTracker tracker(__FUNCTION__)

#endif // TRACE_FUNCTIONS_H
