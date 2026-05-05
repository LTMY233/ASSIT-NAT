#include "rf433_tool.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../chinese_glyphs.h"
#include "../utils.h"
#include <math.h>

ModuleInterface* createRf433Tool() { return new Rf433Tool(); }

const char* Rf433Tool::getTitle() const {
    switch (state) {
        case RF_MENU:        return "433MHz Tool";
        case RF_SCAN:        return "433 Scan";
        case RF_BROWSE:      return "Captures";
        case RF_SEND:        return "433 Send";
        case RF_RAW_EDIT:    return "Raw Edit";
        case RF_DEVDB:       return "Device DB";
        case RF_TEMP_SENSOR: return "Temp Sensor";
        default: return "433MHz";
    }
}

RefreshMode Rf433Tool::getRefreshMode() const {
    return (state == RF_SCAN || state == RF_TEMP_SENSOR)
           ? REFRESH_CONTINUOUS : REFRESH_ON_DEMAND;
}

// ============================================================
// Lifecycle
// ============================================================
void Rf433Tool::init() {
    memset(codes, 0, sizeof(codes));
    memset(scanRaw, 0, sizeof(scanRaw));
    memset(tempType, 0, sizeof(tempType));
    codeCount = 0; codeCursor = 0; menuCursor = 0;
    rxActive = false; txActive = false;
    scanLastVal = 0; scanLastBits = 0; scanLastProto = 0;
    scanSignalCnt = 0; scanRawCnt = 0; scanAvgUs = 0;
    txCode = 0x556688; txBits = 24; txProto = 1;
    txPulseUs = 320; txEditPos = 0; txEditMode = 0; txCount = 0;
    dbCursor = 0; dbDetail = false;
    tempVal = 0; humidity = 0; tempTs = 0;
    state = RF_MENU;
    mScrollY = mScrollYTarget = 0;
    mSelY = mSelYTarget = 0;
    mSelW = mSelWTarget = 0;
    mAnimLastMs = 0;
}

void Rf433Tool::enter() {
    state = RF_MENU;
    menuCursor = 0;
    disableRX();
    disableTX();
    mScrollY = mScrollYTarget = 0;
    mSelY = mSelYTarget = 0;
    mSelW = mSelWTarget = 0;
    mAnimLastMs = millis();
    displayMgr.setDirty();
}

void Rf433Tool::exit() {
    disableRX();
    disableTX();
    state = RF_MENU;
}

// ============================================================
// RX/TX management
// ============================================================
void Rf433Tool::enableRX() {
    if (rxActive) return;
    disableTX();
    pinMode(PIN_RX433, INPUT);
    rcSwitch.enableReceive(PIN_RX433);
    rxActive = true;
}

void Rf433Tool::disableRX() {
    if (!rxActive) return;
    rcSwitch.disableReceive();
    rxActive = false;
}

void Rf433Tool::enableTX() {
    if (txActive) return;
    disableRX();
    pinMode(PIN_TX433, OUTPUT);
    digitalWrite(PIN_TX433, LOW);
    rcSwitch.enableTransmit(PIN_TX433);
    txActive = true;
}

void Rf433Tool::disableTX() {
    if (!txActive) return;
    rcSwitch.disableTransmit();
    pinMode(PIN_TX433, INPUT);
    txActive = false;
}

// ============================================================
// Helpers
// ============================================================
void Rf433Tool::addCode(unsigned long val, uint8_t bits, uint8_t proto) {
    if (codeCount >= RF433_MAX_CODES) {
        // Shift buffer, drop oldest
        for (uint8_t i = 0; i < RF433_MAX_CODES - 1; i++)
            codes[i] = codes[i+1];
        codeCount = RF433_MAX_CODES - 1;
    }
    codes[codeCount].value     = val;
    codes[codeCount].bitLength = bits;
    codes[codeCount].protocol  = proto;
    codes[codeCount].timestamp = millis();
    codeCount++;
    codeCursor = codeCount - 1;
}

void Rf433Tool::analyzePulses() {
    if (scanRawCnt < 2) { scanAvgUs = 0; return; }
    uint32_t sum = 0;
    for (uint8_t i = 0; i < scanRawCnt; i++)
        sum += abs(scanRaw[i]);
    scanAvgUs = sum / scanRawCnt;
}

const char* Rf433Tool::guessProtocol(uint16_t avgUs) {
    if (avgUs == 0) return "Unknown";
    if (avgUs >= 300 && avgUs <= 400) return "EV1527/PT2262";
    if (avgUs >= 500 && avgUs <= 700) return "SC5262/HS2262";
    if (avgUs >= 800 && avgUs <= 1000) return "HX2262";
    if (avgUs >= 150 && avgUs <= 250) return "HT12E";
    if (avgUs >= 100 && avgUs <= 180) return "ASK/OOK short";
    return "Custom OOK";
}

const char* Rf433Tool::protoName(uint8_t proto) {
    switch (proto) {
        case 1:  return "EV1527";
        case 2:  return "PT2262";
        case 3:  return "SC5262";
        case 4:  return "HX2262";
        case 5:  return "HT12E";
        case 6:  return "GenericASK";
        case 7:  return "Custom";
        default: return "?";
    }
}

void Rf433Tool::bitsToStr(unsigned long val, uint8_t bits, char* buf, size_t sz) {
    if (bits == 0) { buf[0] = '\0'; return; }
    if (bits > 32) bits = 32;
    if (sz < bits + 1) return;
    for (int i = bits - 1; i >= 0; i--)
        buf[bits - 1 - i] = (val & (1UL << i)) ? '1' : '0';
    buf[bits] = '\0';
}

void Rf433Tool::tryDecodeTemp(unsigned long val, uint8_t bits, uint8_t proto) {
    // EV1527-based temp sensor: 24 bits, first 4=channel, next 12=temp*10, last 8=CRC
    if (bits < 20) return;
    uint16_t raw = (val >> 8) & 0xFFF;
    float t = raw * 0.1f - 50.0f;  // typical offset
    if (t > -30 && t < 70) {
        tempVal = t;
        humidity = (val >> 4) & 0x0F;
        strCopySafe(tempType, "EV1527/DHT", sizeof(tempType));
        tempTs = millis();
    }
}

void Rf433Tool::recomputeMenuTargets() {
    const uint8_t n = 6;
    if (menuCursor < 3) {
        mScrollYTarget = 0;
    } else if (menuCursor >= n - 1) {
        mScrollYTarget = (float)(n - 3) * MENU_ROW_HEIGHT;
    } else {
        mScrollYTarget = (float)(menuCursor - 1) * MENU_ROW_HEIGHT;
    }
    mSelYTarget = (float)menuCursor * MENU_ROW_HEIGHT - mScrollYTarget + MENU_TOP_PAD;
    mSelWTarget = 0;
}

// ============================================================
// Update
// ============================================================
void Rf433Tool::update() {
    // Menu animation
    if (state == RF_MENU) {
        uint32_t now = millis();
        float dt = (now - mAnimLastMs) / 1000.0f;
        mAnimLastMs = now;
        if (dt <= 0.0f || dt > 0.05f) dt = 0.05f;
        float t = 1.0f - expf(-8.0f * dt);
        mScrollY += (mScrollYTarget - mScrollY) * t;
        mSelY    += (mSelYTarget - mSelY) * t;
        mSelW    += (mSelWTarget - mSelW) * t;
        if (fabsf(mScrollYTarget - mScrollY) < 0.2f) mScrollY = mScrollYTarget;
        if (fabsf(mSelYTarget - mSelY) < 0.2f) mSelY = mSelYTarget;
        if (fabsf(mSelWTarget - mSelW) < 0.2f) mSelW = mSelWTarget;
        if (fabsf(mScrollYTarget - mScrollY) > 0.1f || fabsf(mSelYTarget - mSelY) > 0.1f || fabsf(mSelWTarget - mSelW) > 0.1f)
            displayMgr.setDirty();
    }

    if (state == RF_SCAN || state == RF_TEMP_SENSOR) {
        if (!rcSwitch.available()) return;
        unsigned long val = rcSwitch.getReceivedValue();
        uint8_t bits = rcSwitch.getReceivedBitlength();
        uint8_t proto = rcSwitch.getReceivedProtocol();
        unsigned int* raw = rcSwitch.getReceivedRawdata();

        if (state == RF_SCAN) {
            scanLastVal   = val;
            scanLastBits  = bits;
            scanLastProto = proto;
            scanRawCnt = 0;
            if (raw) {
                for (int i = 0; i < RF433_MAX_PULSES && raw[i] > 0; i++) {
                    scanRaw[i] = raw[i];
                    scanRawCnt++;
                }
            }
            analyzePulses();
            scanSignalCnt++;
            addCode(val, bits, proto);
        } else {
            tryDecodeTemp(val, bits, proto);
            addCode(val, bits, proto);
        }
        rcSwitch.resetAvailable();
        displayMgr.setDirty();
    }
}

// ============================================================
// Button handling
// ============================================================
void Rf433Tool::handleButton(ButtonEvent ev) {
    switch (state) {
        case RF_MENU:
            if (ev == BTN_UP_SHORT && menuCursor > 0) { menuCursor--; recomputeMenuTargets(); }
            if (ev == BTN_DOWN_SHORT && menuCursor < 5) { menuCursor++; recomputeMenuTargets(); }
            if (ev == BTN_UP_REPEAT && menuCursor > 0) { menuCursor--; recomputeMenuTargets(); }
            if (ev == BTN_DOWN_REPEAT && menuCursor < 5) { menuCursor++; recomputeMenuTargets(); }
            if (ev == BTN_OK_SHORT) {
                disableRX(); disableTX();
                switch (menuCursor) {
                    case 0: state = RF_SCAN;   enableRX(); scanSignalCnt = 0; break;
                    case 1: state = RF_BROWSE; codeCursor = codeCount > 0 ? codeCount-1 : 0; break;
                    case 2: state = RF_SEND; enableTX();
                            if (codeCount > 0) {
                                txCode = codes[codeCursor].value;
                                txBits = codes[codeCursor].bitLength;
                                txProto = codes[codeCursor].protocol;
                            }
                            txCount = 0; break;
                    case 3: state = RF_RAW_EDIT; enableTX(); txCount = 0; break;
                    case 4: state = RF_DEVDB; dbCursor = 0; dbDetail = false; break;
                    case 5: state = RF_TEMP_SENSOR; enableRX(); tempVal = 0; tempTs = 0; break;
                }
            }
            break;

        case RF_SCAN:
            if (ev == BTN_OK_SHORT) {
                scanSignalCnt = 0; scanLastVal = 0;
                scanRawCnt = 0; scanAvgUs = 0;
            }
            break;

        case RF_BROWSE:
            if (ev == BTN_UP_SHORT && codeCursor > 0) codeCursor--;
            if (ev == BTN_DOWN_SHORT && codeCursor < codeCount - 1) codeCursor++;
            if (ev == BTN_OK_SHORT && codeCount > 0) {
                // Send immediately like 射频管家
                enableTX();
                rcSwitch.setProtocol(codes[codeCursor].protocol);
                rcSwitch.setRepeatTransmit(5);
                rcSwitch.send(codes[codeCursor].value, codes[codeCursor].bitLength);
                txCount++;
                disableTX();
            }
            break;

        case RF_SEND:
            if (ev == BTN_OK_SHORT) {
                rcSwitch.setProtocol(txProto);
                rcSwitch.setRepeatTransmit(5);
                rcSwitch.send(txCode, txBits);
                txCount++;
            }
            if (ev == BTN_UP_SHORT && txProto < 7) txProto++;
            if (ev == BTN_DOWN_SHORT && txProto > 1) txProto--;
            if (ev == BTN_UP_LONG) { state = RF_MENU; disableTX(); }
            break;

        case RF_RAW_EDIT:
            if (ev == BTN_UP_LONG)   { txEditMode = (txEditMode + 1) % 3; txEditPos = 0; }
            if (ev == BTN_DOWN_LONG) { txEditMode = (txEditMode + 2) % 3; txEditPos = 0; }
            switch (txEditMode) {
                case 0: // bit edit
                    if (ev == BTN_UP_SHORT && txEditPos < txBits - 1) txEditPos++;
                    if (ev == BTN_DOWN_SHORT && txEditPos > 0) txEditPos--;
                    if (ev == BTN_OK_SHORT) {
                        txCode ^= (1UL << (txBits - 1 - txEditPos));
                    }
                    break;
                case 1: // protocol
                    if (ev == BTN_UP_SHORT && txProto < 7) txProto++;
                    if (ev == BTN_DOWN_SHORT && txProto > 1) txProto--;
                    break;
                case 2: // pulse
                    if (ev == BTN_UP_SHORT) txPulseUs += 10;
                    if (ev == BTN_DOWN_SHORT && txPulseUs > 10) txPulseUs -= 10;
                    break;
            }
            if (ev == BTN_OK_DOUBLE || ev == BTN_OK_LONG) { /* do nothing - handled by core */ }
            if (ev == BTN_UP_LONG || ev == BTN_DOWN_LONG) { /* handled above */ }
            break;

        case RF_DEVDB:
            if (dbDetail) {
                if (ev == BTN_UP_SHORT || ev == BTN_DOWN_SHORT) dbDetail = false;
                if (ev == BTN_OK_SHORT) {
                    enableTX();
                    rcSwitch.setProtocol(txProto);
                    rcSwitch.setRepeatTransmit(5);
                    rcSwitch.send(txCode, txBits);
                    txCount++;
                    disableTX();
                }
            } else {
                if (ev == BTN_UP_SHORT && dbCursor > 0) dbCursor--;
                if (ev == BTN_DOWN_SHORT && dbCursor < RF433_DB_COUNT - 1) dbCursor++;
                if (ev == BTN_OK_SHORT) dbDetail = true;
            }
            break;

        case RF_TEMP_SENSOR:
            if (ev == BTN_OK_SHORT) { tempVal = 0; humidity = 0; tempTs = 0; }
            break;
    }
    displayMgr.setDirty();
}

// ============================================================
// Draw dispatch
// ============================================================
void Rf433Tool::draw(U8G2& u8g2) {
    switch (state) {
        case RF_MENU:        drawMenu(u8g2); break;
        case RF_SCAN:        drawScan(u8g2); break;
        case RF_BROWSE:      drawBrowse(u8g2); break;
        case RF_SEND:        drawSend(u8g2); break;
        case RF_RAW_EDIT:    drawRawEdit(u8g2); break;
        case RF_DEVDB:       drawDevDb(u8g2); break;
        case RF_TEMP_SENSOR: drawTemp(u8g2); break;
    }
}

// ============================================================
// Draw — RF_MENU
// ============================================================
void Rf433Tool::drawMenu(U8G2& u8g2) {
    static const char* itemCN[6] = {
        "扫描433",
        "已捕获码",
        "发送433",
        "原始编辑",
        "设备库",
        "温度传感",
    };

    const int cnBaseOff = MENU_ROW_HEIGHT - 2;

    // 1. Draw text items
    for (uint8_t i = 0; i < 6; i++) {
        int y = (int)(i * MENU_ROW_HEIGHT - mScrollY + MENU_TOP_PAD + cnBaseOff);
        if (y < -14 || y > OLED_HEIGHT + 16) continue;
        drawCN(u8g2, MENU_TEXT_X, y, itemCN[i]);
    }

    // 2. Scrollbar
    u8g2.drawVLine(MENU_SCROLLBAR_X + 1, 0, OLED_HEIGHT);
    float sbH = OLED_HEIGHT * 3.0f / 6.0f;
    float sbY = (OLED_HEIGHT - sbH) * menuCursor / 5.0f;
    u8g2.drawBox(MENU_SCROLLBAR_X, (int)sbY, MENU_SCROLLBAR_W, max(4, (int)sbH));

    // 3. XOR selection box
    int tw = cnStrWidth(itemCN[menuCursor]);
    mSelWTarget = tw + MENU_SEL_PAD_X * 2;
    int bw = max(20, (int)mSelW);
    u8g2.setDrawColor(2);
    u8g2.drawRBox(MENU_TEXT_X - MENU_SEL_PAD_X, (int)mSelY, bw, MENU_ROW_HEIGHT - 1, MENU_SEL_RADIUS);
    u8g2.setDrawColor(1);

    // 4. Footer
    u8g2.setFont(FONT_SMALL);
    u8g2.drawStr(0, 63, "OK:enter  UP/DN");
}

// ============================================================
// Draw — RF_SCAN
// ============================================================
void Rf433Tool::drawScan(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, "扫描433");

    if (scanSignalCnt == 0) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(10, 35, "Scanning 433MHz...");
    } else {
        char buf[32];
        // Protocol line with name
        snprintf(buf, sizeof(buf), "Proto: %s", protoName(scanLastProto));
        u8g2.drawStr(2, 20, buf);
        // Pulse width
        snprintf(buf, sizeof(buf), "Pulse: %dus", scanAvgUs);
        u8g2.drawStr(2, 30, buf);
        // Decimal value
        snprintf(buf, sizeof(buf), "DEC: %lu", scanLastVal);
        u8g2.drawStr(2, 40, buf);
        // Bits + binary preview (first 8 bits)
        char bin[17]; bitsToStr(scanLastVal, scanLastBits > 16 ? 16 : scanLastBits, bin, sizeof(bin));
        snprintf(buf, sizeof(buf), "Bits:%d BIN:%s", scanLastBits, bin);
        u8g2.setFont(FONT_SMALL);
        u8g2.drawStr(2, 53, buf);
    }
    u8g2.setFont(FONT_DATA);
    char foot[24];
    snprintf(foot, sizeof(foot), "Sigs:%lu OK:clear", scanSignalCnt);
    u8g2.drawStr(0, 63, foot);
}

// ============================================================
// Draw — RF_BROWSE
// ============================================================
void Rf433Tool::drawBrowse(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, "已捕获码");

    if (codeCount == 0) {
        drawCN(u8g2, 10, 38, "无捕获信号");
    } else {
        for (uint8_t i = 0; i < 3 && i < codeCount; i++) {
            uint8_t y = 22 + i * 15;
            uint8_t idx = codeCount - 1 - i;
            if (idx == codeCursor) {
                u8g2.drawBox(0, y - 11, OLED_WIDTH, 14);
                u8g2.setDrawColor(0);
            }
            char buf[40];
            snprintf(buf, sizeof(buf), "#%d: %lu %db %s",
                     idx + 1, codes[idx].value, codes[idx].bitLength,
                     protoName(codes[idx].protocol));
            u8g2.setFont(FONT_DATA);
            u8g2.drawStr(2, y, buf);
            if (idx == codeCursor) u8g2.setDrawColor(1);
        }
    }
    u8g2.setFont(FONT_SMALL);
    char foot[24];
    snprintf(foot, sizeof(foot), "%d codes  OK:send", codeCount);
    u8g2.drawStr(0, 63, foot);
}

// ============================================================
// Draw — RF_SEND
// ============================================================
void Rf433Tool::drawSend(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, "发送433");

    char buf[32];
    snprintf(buf, sizeof(buf), "Code: %lu", txCode);
    u8g2.drawStr(2, 20, buf);
    snprintf(buf, sizeof(buf), "Bits: %d", txBits);
    u8g2.drawStr(2, 30, buf);
    snprintf(buf, sizeof(buf), "Proto: %s (#%d)", protoName(txProto), txProto);
    u8g2.drawStr(2, 40, buf);
    snprintf(buf, sizeof(buf), "Pulse: %dus", txPulseUs);
    u8g2.drawStr(2, 50, buf);
    u8g2.setFont(FONT_SMALL);
    snprintf(buf, sizeof(buf), "TX:%lu  OK:send  UP/DN:proto", txCount);
    u8g2.drawStr(0, 63, buf);
}

// ============================================================
// Draw — RF_RAW_EDIT
// ============================================================
void Rf433Tool::drawRawEdit(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, "原始编辑");

    // Binary display
    char binBuf[40];
    bitsToStr(txCode, txBits, binBuf, sizeof(binBuf));
    u8g2.setFont(FONT_SMALL);
    uint8_t y = 20;
    for (uint8_t row = 0; row < 3 && row * 12 < txBits; row++) {
        for (uint8_t col = 0; col < 12 && (row*12+col) < txBits; col++) {
            uint8_t idx = row * 12 + col;
            char c[2] = {binBuf[idx], '\0'};
            int x = 4 + col * 10;
            if (idx == txEditPos && txEditMode == 0) {
                u8g2.drawBox(x - 1, y - 6, 9, 8);
                u8g2.setDrawColor(0);
                u8g2.drawStr(x, y, c);
                u8g2.setDrawColor(1);
            } else {
                u8g2.drawStr(x, y, c);
            }
        }
        y += 10;
    }

    char buf[40];
    snprintf(buf, sizeof(buf), "P:%d Bits:%d PUL:%d",
             txProto, txBits, txPulseUs);
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(2, 50, buf);

    const char* modeStr[3] = {"Bit", "Proto", "Pulse"};
    snprintf(buf, sizeof(buf), "Mode:%s OK:send", modeStr[txEditMode]);
    u8g2.drawStr(0, 63, buf);
}

// ============================================================
// Draw — RF_DEVDB
// ============================================================
void Rf433Tool::drawDevDb(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, "设备库");

    // Built-in device database
    static const struct { const char* name; unsigned long code; uint8_t bits; uint8_t proto; } db[RF433_DB_COUNT] = {
        {"EV1527 ID1",  0x556688, 24, 1},
        {"EV1527 ID2",  0x887766, 24, 1},
        {"PT2262 A1",   0x123456, 24, 2},
        {"PT2262 A2",   0x654321, 24, 2},
        {"SC5262 BTN1", 0x112233, 24, 3},
        {"SC5262 BTN2", 0x332211, 24, 3},
        {"HX2262 ChA",  0x445566, 24, 4},
        {"HT12E D1",    0x778899, 12, 5},
        {"Generic 12b", 0x000FFF, 12, 1},
        {"Doorbell 1",  0x101010, 24, 1},
        {"Garage Open", 0x202020, 24, 2},
        {"Alarm Ctrl",  0x303030, 24, 3},
    };

    if (dbDetail) {
        auto& d = db[dbCursor];
        u8g2.drawStr(2, 22, d.name);
        char buf[32];
        snprintf(buf, sizeof(buf), "DEC:%lu", d.code);
        u8g2.drawStr(2, 34, buf);
        char bin[40];
        bitsToStr(d.code, d.bits, bin, sizeof(bin));
        u8g2.drawStr(2, 44, bin);
        snprintf(buf, sizeof(buf), "Bits:%d P:%d", d.bits, d.proto);
        u8g2.drawStr(2, 54, buf);
        u8g2.drawStr(0, 63, "OK:send  UP/DN:back");
    } else {
        for (uint8_t i = 0; i < 4 && i < RF433_DB_COUNT; i++) {
            uint8_t y = 22 + i * 11;
            if (i == dbCursor) {
                u8g2.drawBox(0, y - 8, OLED_WIDTH, 10);
                u8g2.setDrawColor(0);
            }
            u8g2.setFont(FONT_BODY);
            u8g2.drawStr(4, y, db[i].name);
            if (i == dbCursor) u8g2.setDrawColor(1);
        }
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(0, 63, "OK:detail  UP/DN");
    }
}

// ============================================================
// Draw — RF_TEMP_SENSOR
// ============================================================
void Rf433Tool::drawTemp(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, "温度传感");

    if (tempVal == 0 && tempTs == 0) {
        drawCN(u8g2, 10, 38, "监听中...");
    } else {
        u8g2.setFont(FONT_BIG);
        char buf[10];
        snprintf(buf, sizeof(buf), "%.1fC", tempVal);
        uint8_t tw = u8g2.getStrWidth(buf);
        u8g2.drawStr((OLED_WIDTH - tw) / 2, 35, buf);

        u8g2.setFont(FONT_DATA);
        snprintf(buf, sizeof(buf), "H:%d%% T:%s", humidity, tempType);
        u8g2.drawStr(2, 50, buf);

        uint32_t elapsed = (millis() - tempTs) / 1000;
        snprintf(buf, sizeof(buf), "Age:%lus", elapsed);
        u8g2.drawStr(2, 58, buf);
    }
    u8g2.drawStr(0, 63, "OK:clear");
}
