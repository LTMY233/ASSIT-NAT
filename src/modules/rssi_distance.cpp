#include "../chinese_glyphs.h"
#include "rssi_distance.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"

ModuleInterface* createRssiDistance() { return new RssiDistance(); }

void RssiDistance::init() {
    n = 2.5f;
    refRssi = -40;
    currentRssi = -60;
    distance = 0;
    editMode = 0;
}

void RssiDistance::enter() {
    editMode = 0;
    calcDistance();
    displayMgr.setDirty();
}

void RssiDistance::exit() {}

void RssiDistance::calcDistance() {
    // Log-distance path loss model
    // distance = 10^((refRssi - rssi) / (10 * n))
    float exponent = (float)(refRssi - currentRssi) / (10.0f * n);
    distance = powf(10.0f, exponent);
}

void RssiDistance::update() {
    // Get current RSSI from WiFi
    if (wifiMgr.isConnected()) {
        int8_t rssi = wifiMgr.rssi();
        if (rssi != currentRssi && rssi < 0) {
            currentRssi = rssi;
            calcDistance();
            displayMgr.setDirty();
        }
    }
}

void RssiDistance::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            editMode = (editMode + 1) % 3;  // Toggle edit mode
            displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            if (editMode == 1) { n += 0.1f; if (n > 5.0f) n = 5.0f; }
            else if (editMode == 2) { refRssi += 1; if (refRssi > -20) refRssi = -20; }
            calcDistance();
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (editMode == 1) { n -= 0.1f; if (n < 1.0f) n = 1.0f; }
            else if (editMode == 2) { refRssi -= 1; if (refRssi < -80) refRssi = -80; }
            calcDistance();
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void RssiDistance::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    u8g2.setFont(FONT_BIG);
    char buf[16];
    if (distance < 1.0f) {
        snprintf(buf, sizeof(buf), "%.1fm", distance);
    } else if (distance < 100) {
        snprintf(buf, sizeof(buf), "%.1fm", distance);
    } else {
        snprintf(buf, sizeof(buf), ">100m");
    }
    uint8_t tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 28, buf);

    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "RSSI:%d dBm  A:%d dBm", currentRssi, refRssi);
    u8g2.drawStr(2, 42, buf);

    snprintf(buf, sizeof(buf), "n=%.1f A=%d  确认=切换",
             n, refRssi);
    drawCN(u8g2, 0, 63, buf);
}
