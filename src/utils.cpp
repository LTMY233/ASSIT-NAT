#include "utils.h"

void macToStr(const uint8_t* mac, char* buf, size_t bufsize) {
    if (bufsize < 18) return;
    snprintf(buf, bufsize, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool strToMac(const char* str, uint8_t* mac) {
    int vals[6];
    if (sscanf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
               &vals[0], &vals[1], &vals[2], &vals[3], &vals[4], &vals[5]) == 6) {
        for (int i = 0; i < 6; i++) mac[i] = (uint8_t)vals[i];
        return true;
    }
    if (sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
               &vals[0], &vals[1], &vals[2], &vals[3], &vals[4], &vals[5]) == 6) {
        for (int i = 0; i < 6; i++) mac[i] = (uint8_t)vals[i];
        return true;
    }
    return false;
}

void ipToStr(IPAddress ip, char* buf, size_t bufsize) {
    snprintf(buf, bufsize, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

void hexDump(const uint8_t* data, size_t len, char* buf, size_t bufsize, bool spaces) {
    size_t pos = 0;
    for (size_t i = 0; i < len && pos + 3 < bufsize; i++) {
        if (spaces && i > 0) buf[pos++] = ' ';
        snprintf(buf + pos, bufsize - pos, "%02X", data[i]);
        pos += 2;
    }
    buf[pos] = '\0';
}

uint8_t rssiToPercent(int8_t rssi) {
    if (rssi >= -50) return 100;
    if (rssi <= -100) return 0;
    return (uint8_t)(2 * (rssi + 100));
}

const char* rssiToBar(int8_t rssi) {
    if (rssi >= -50) return "████";
    if (rssi >= -60) return "███▌";
    if (rssi >= -70) return "███ ";
    if (rssi >= -80) return "██  ";
    return "█   ";
}

// ---- LittleFS ----

bool lfsInit() {
    return LittleFS.begin();
}

bool lfsExists(const char* path) {
    return LittleFS.exists(path);
}

size_t lfsFileSize(const char* path) {
    File f = LittleFS.open(path, "r");
    if (!f) return 0;
    size_t s = f.size();
    f.close();
    return s;
}

bool lfsReadFile(const char* path, uint8_t* buf, size_t maxlen, size_t* outlen) {
    File f = LittleFS.open(path, "r");
    if (!f) return false;
    size_t len = f.read(buf, maxlen);
    f.close();
    if (outlen) *outlen = len;
    return true;
}

bool lfsWriteFile(const char* path, const uint8_t* data, size_t len) {
    File f = LittleFS.open(path, "w");
    if (!f) return false;
    size_t written = f.write(data, len);
    f.close();
    return written == len;
}

bool lfsAppendFile(const char* path, const uint8_t* data, size_t len) {
    File f = LittleFS.open(path, "a");
    if (!f) return false;
    size_t written = f.write(data, len);
    f.close();
    return written == len;
}

bool lfsDeleteFile(const char* path) {
    return LittleFS.remove(path);
}

void lfsListFiles(void (*callback)(const char* name, size_t size)) {
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        callback(dir.fileName().c_str(), dir.fileSize());
    }
}

// ---- String tools ----

void strCopySafe(char* dst, const char* src, size_t dstsize) {
    if (dstsize == 0) return;
    size_t i;
    for (i = 0; i < dstsize - 1 && src[i]; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

bool strStartsWith(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str != *prefix) return false;
        str++; prefix++;
    }
    return true;
}

char* itoaSimple(int32_t val, char* buf, int base) {
    if (base < 2 || base > 16) { buf[0] = '\0'; return buf; }
    char* p = buf;
    if (val < 0 && base == 10) { *p++ = '-'; val = -val; }
    if (val == 0) { *p++ = '0'; *p = '\0'; return buf; }
    char tmp[12]; int i = 0;
    while (val > 0) {
        int d = val % base;
        tmp[i++] = (d < 10) ? ('0' + d) : ('A' + d - 10);
        val /= base;
    }
    while (i > 0) *p++ = tmp[--i];
    *p = '\0';
    return buf;
}

// ---- Time tools ----
// Simple UTC epoch formatting, no time.h dependency

static const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static bool isLeapYear(uint16_t y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

// Break epoch into YMDhms (UTC)
static void breakEpoch(uint32_t epoch, uint16_t& year, uint8_t& month, uint8_t& day,
                       uint8_t& hour, uint8_t& minute, uint8_t& second) {
    second = epoch % 60; epoch /= 60;
    minute = epoch % 60; epoch /= 60;
    hour   = epoch % 24; epoch /= 24;

    uint16_t days = (uint16_t)epoch;
    year = 1970;
    while (true) {
        uint16_t dy = isLeapYear(year) ? 366 : 365;
        if (days < dy) break;
        days -= dy;
        year++;
    }
    month = 0;
    for (uint8_t m = 0; m < 12; m++) {
        uint8_t dm = daysInMonth[m];
        if (m == 1 && isLeapYear(year)) dm = 29;
        if (days < dm) { month = m + 1; break; }
        days -= dm;
    }
    day = (uint8_t)(days + 1);
}

void formatTime(uint32_t epoch, char* buf, size_t bufsize) {
    uint16_t y; uint8_t mo, d, h, mi, s;
    breakEpoch(epoch, y, mo, d, h, mi, s);
    snprintf(buf, bufsize, "%02d:%02d:%02d", h, mi, s);
}

void formatDate(uint32_t epoch, char* buf, size_t bufsize) {
    uint16_t y; uint8_t mo, d, h, mi, s;
    breakEpoch(epoch, y, mo, d, h, mi, s);
    snprintf(buf, bufsize, "%04d-%02d-%02d", y, mo, d);
}

void formatDateTime(uint32_t epoch, char* buf, size_t bufsize) {
    uint16_t y; uint8_t mo, d, h, mi, s;
    breakEpoch(epoch, y, mo, d, h, mi, s);
    snprintf(buf, bufsize, "%04d-%02d-%02d %02d:%02d:%02d", y, mo, d, h, mi, s);
}
