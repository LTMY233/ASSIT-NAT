#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "config.h"

// ============================================================
// MAC formatting
// ============================================================
// Format uint8_t[6] -> "AA:BB:CC:DD:EE:FF"
void macToStr(const uint8_t* mac, char* buf, size_t bufsize);
// Parse "AA:BB:CC:DD:EE:FF" -> uint8_t[6]
bool strToMac(const char* str, uint8_t* mac);

// ============================================================
// IP formatting
// ============================================================
void ipToStr(IPAddress ip, char* buf, size_t bufsize);

// ============================================================
// Hex dump
// ============================================================
// Format byte array as hex string (debug)
void hexDump(const uint8_t* data, size_t len, char* buf, size_t bufsize, bool spaces = true);

// ============================================================
// RSSI / signal
// ============================================================
// Map RSSI to signal strength %%
uint8_t rssiToPercent(int8_t rssi);
// Get RSSI display char
const char* rssiToBar(int8_t rssi);

// ============================================================
// LittleFS helpers
// ============================================================
bool lfsInit();
bool lfsExists(const char* path);
size_t lfsFileSize(const char* path);
bool lfsReadFile(const char* path, uint8_t* buf, size_t maxlen, size_t* outlen = nullptr);
bool lfsWriteFile(const char* path, const uint8_t* data, size_t len);
bool lfsAppendFile(const char* path, const uint8_t* data, size_t len);
bool lfsDeleteFile(const char* path);
void lfsListFiles(void (*callback)(const char* name, size_t size));

// ============================================================
// String tools
// ============================================================
// Safe string copy, ensures null term
void strCopySafe(char* dst, const char* src, size_t dstsize);
// Compare first n chars (partial match)
bool strStartsWith(const char* str, const char* prefix);
// Simple int->string (avoid snprintf cost)
char* itoaSimple(int32_t val, char* buf, int base = 10);

// ============================================================
// Time tools
// ============================================================
// Format epoch -> "14:30:00"
void formatTime(uint32_t epoch, char* buf, size_t bufsize);
// Format epoch -> "2026-05-04"
void formatDate(uint32_t epoch, char* buf, size_t bufsize);
// Format epoch -> datetime
void formatDateTime(uint32_t epoch, char* buf, size_t bufsize);

// ============================================================
// Math/mapping
// ============================================================
// Clamp value to [lo, hi]
template<typename T> T clamp(T val, T lo, T hi) {
    return val < lo ? lo : (val > hi ? hi : val);
}
// Linear map (no div0 check)
inline int32_t mapRange(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
