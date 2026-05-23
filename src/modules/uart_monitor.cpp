#include "../chinese_glyphs.h"
#include "uart_monitor.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createUartMonitor() { return new UartMonitor(); }

const uint32_t UartMonitor::baudList[8] = {
    9600, 19200, 38400, 57600, 74880, 115200, 230400, 460800
};

void UartMonitor::init() {
    baudRate = 115200;
    rxCount = 0;
    txCount = 0;
    lastUpdate = 0;
    memset(rxBuf, 0, sizeof(rxBuf));
    rxIdx = 0;
    passthrough = false;
    baudIdx = 5;  // 115200
}

void UartMonitor::enter() {
    baudRate = baudList[baudIdx];
    rxCount = 0;
    txCount = 0;
    lastUpdate = millis();
    memset(rxBuf, 0, sizeof(rxBuf));
    rxIdx = 0;
    passthrough = false;
    applyBaud();
    displayMgr.setDirty();
}

void UartMonitor::exit() {
    passthrough = false;
}

void UartMonitor::applyBaud() {
    baudRate = baudList[baudIdx];
    Serial.end();
    delay(50);
    Serial.begin(baudRate);
}

void UartMonitor::update() {
    // Read incoming serial data
    while (Serial.available() > 0) {
        char c = Serial.read();
        rxCount++;
        if (rxIdx < UART_RX_BUF_SIZE - 1) {
            rxBuf[rxIdx++] = c;
            rxBuf[rxIdx] = '\0';
        }
        // In passthrough mode, echo back
        if (passthrough) {
            Serial.write(c);
            txCount++;
        }
        displayMgr.setDirty();
    }

    // Rotate buffer if full
    if (rxIdx >= UART_RX_BUF_SIZE - 1) {
        memmove(rxBuf, rxBuf + UART_RX_BUF_SIZE / 2, UART_RX_BUF_SIZE / 2);
        rxIdx = UART_RX_BUF_SIZE / 2;
    }
}

void UartMonitor::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            passthrough = !passthrough;
            displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            if (baudIdx < 7) {
                baudIdx++;
                applyBaud();
            }
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (baudIdx > 0) {
                baudIdx--;
                applyBaud();
            }
            displayMgr.setDirty();
            break;
        case BTN_UP_DOUBLE:
            // Send test string
            Serial.println("ESP8266 UART Test\r\n");
            txCount += 18;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void UartMonitor::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // Baud rate
    u8g2.setFont(FONT_BODY);
    char buf[32];
    snprintf(buf, sizeof(buf), "Baud:%lu ", baudRate);
    u8g2.drawStr(2, 24, buf);
    drawCN(u8g2, 2 + u8g2.getStrWidth(buf), 25, passthrough ? "直传" : "监听");

    // RX/TX counts
    snprintf(buf, sizeof(buf), "RX:%lu", rxCount);
    u8g2.drawStr(2, 34, buf);

    snprintf(buf, sizeof(buf), "TX:%lu", txCount);
    u8g2.drawStr(70, 34, buf);

    // Recent data preview
    u8g2.setFont(FONT_SMALL);
    if (rxIdx > 0) {
        // Show last ~20 chars
        uint8_t start = (rxIdx > 20) ? rxIdx - 20 : 0;
        char preview[22];
        uint8_t len = rxIdx - start;
        if (len > 20) len = 20;
        memcpy(preview, rxBuf + start, len);
        preview[len] = '\0';
        // Replace non-printable chars
        for (uint8_t i = 0; i < len; i++) {
            if (preview[i] < 32 && preview[i] != '\n' && preview[i] != '\r')
                preview[i] = '.';
        }
        u8g2.drawStr(2, 46, preview);
    }

    // Footer
    u8g2.setFont(FONT_DATA);
}
