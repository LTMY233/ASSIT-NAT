#include "http_client.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

const char* HttpClient::presetUrls[4] = {
    "http://httpbin.org/get",
    "http://neverssl.com",
    "http://example.com",
    "http://httpbin.org/ip"
};

ModuleInterface* createHttpClient() { return new HttpClient(); }

char HttpClient::cycleChar(char c, bool up) {
    if (up) {
        if (c < 32 || c > 126) return ' ';
        if (c >= 126) return ' ';
        return c + 1;
    } else {
        if (c < 32 || c > 126) return 'a';
        if (c <= 32) return 126;
        return c - 1;
    }
}

void HttpClient::init() {
    strCopySafe(url, presetUrls[0], sizeof(url));
    urlLen = strlen(url);
    urlIdx = 0;
    editPos = 0;
    statusCode = 0;
    responseSize = 0;
    loaded = false;
    fetching = false;
    strCopySafe(status, "Ready", sizeof(status));
}

void HttpClient::enter() {
    loaded = false;
    fetching = false;
    strCopySafe(status, "Ready", sizeof(status));
    displayMgr.setDirty();
}

void HttpClient::exit() {}

void HttpClient::selectPreset(uint8_t idx) {
    urlIdx = idx;
    strCopySafe(url, presetUrls[idx], sizeof(url));
    urlLen = strlen(url);
    editPos = 0;
    loaded = false;
    fetching = false;
    statusCode = 0;
    responseSize = 0;
    strCopySafe(status, "Ready", sizeof(status));
}

void HttpClient::doFetch() {
    fetching = true;
    loaded = false;
    statusCode = 0;
    responseSize = 0;
    strCopySafe(status, "Connecting...", sizeof(status));
    displayMgr.setDirty();

    // Parse host and path from URL
    char host[48] = {0};
    char path[64] = {"/"};
    uint16_t port = 80;

    const char* p = url;
    if (strStartsWith(url, "http://")) {
        p = url + 7;
    } else if (strStartsWith(url, "https://")) {
        strCopySafe(status, "No HTTPS", sizeof(status));
        fetching = false;
        displayMgr.setDirty();
        return;
    }

    // Extract host
    const char* slash = strchr(p, '/');
    const char* colon = strchr(p, ':');
    if (colon && (!slash || colon < slash)) {
        // Has port
        size_t hostLen = colon - p;
        if (hostLen >= sizeof(host)) hostLen = sizeof(host) - 1;
        strncpy(host, p, hostLen);
        host[hostLen] = '\0';
        port = atoi(colon + 1);
    } else if (slash) {
        size_t hostLen = slash - p;
        if (hostLen >= sizeof(host)) hostLen = sizeof(host) - 1;
        strncpy(host, p, hostLen);
        host[hostLen] = '\0';
    } else {
        strncpy(host, p, sizeof(host) - 1);
    }

    if (slash) strCopySafe(path, slash, sizeof(path));

    WiFiClient client;
    client.setTimeout(5000);

    if (!client.connect(host, port)) {
        strCopySafe(status, "Conn Failed", sizeof(status));
        fetching = false;
        displayMgr.setDirty();
        return;
    }

    // Send HTTP request
    client.print("GET ");
    client.print(path);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(host);
    client.println("User-Agent: ESP8266-Toolkit");
    client.println("Connection: close");
    client.println();

    // Read response
    unsigned long start = millis();
    char buf[128];
    int bufIdx = 0;
    bool headersDone = false;
    int contentLength = -1;

    while (client.connected() || client.available()) {
        if (millis() - start > 8000) break;
        while (client.available()) {
            char c = client.read();
            if (bufIdx < 127) buf[bufIdx++] = c;
            else { responseSize++; continue; }
            buf[bufIdx] = '\0';

            if (!headersDone && strstr(buf, "\r\n\r\n")) {
                headersDone = true;
                // Parse status line
                char* statusLine = buf;
                char* sp = strchr(statusLine, ' ');
                if (sp) statusCode = atoi(sp + 1);

                // Parse Content-Length
                char* cl = strstr(buf, "Content-Length:");
                if (cl) contentLength = atoi(cl + 15);
                responseSize = 0;
            } else if (headersDone) {
                responseSize++;
            }
        }
    }

    client.stop();

    loaded = true;
    if (statusCode > 0) {
        snprintf(status, sizeof(status), "HTTP %d", statusCode);
    } else {
        strCopySafe(status, "No Response", sizeof(status));
    }
    fetching = false;
}

void HttpClient::update() {}

void HttpClient::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            doFetch();
            break;
        case BTN_UP_SHORT:
            urlIdx = (urlIdx + 1) % 4;
            selectPreset(urlIdx);
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            urlIdx = (urlIdx + 3) % 4;
            selectPreset(urlIdx);
            displayMgr.setDirty();
            break;
        case BTN_OK_LONG:
            editPos = (editPos + 1) % urlLen;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void HttpClient::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "HTTP Client");

    // URL (possibly truncated)
    char dispUrl[22];
    if (urlLen > 21) {
        strncpy(dispUrl, url, 20); dispUrl[20] = '~'; dispUrl[21] = '\0';
    } else {
        strCopySafe(dispUrl, url, sizeof(dispUrl));
    }
    u8g2.drawStr(0, 24, dispUrl);

    u8g2.drawHLine(0, 30, OLED_WIDTH);

    if (fetching) {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(0, 44, "Fetching...");
    } else if (loaded) {
        u8g2.setFont(FONT_DATA);
        char buf[32];
        snprintf(buf, sizeof(buf), "Status: %s", status);
        u8g2.drawStr(0, 40, buf);
        snprintf(buf, sizeof(buf), "Size: %d bytes", responseSize);
        u8g2.drawStr(0, 52, buf);
    } else {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(0, 40, "Presets (UP/DN):");
        for (uint8_t i = 0; i < 4; i++) {
            char shortUrl[22];
            const char* src = presetUrls[i];
            if (strlen(src) > 20) {
                strncpy(shortUrl, src, 19); shortUrl[19] = '~'; shortUrl[20] = '\0';
            } else {
                strCopySafe(shortUrl, src, sizeof(shortUrl));
            }
            if (i == urlIdx) {
                u8g2.drawStr(8, 52 + (i/2)*10, ">");
            }
        }
    }

    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 63, "OK=fetch  UP/DN=preset");
}
