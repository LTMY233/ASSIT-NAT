#include "rf433_tool.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../chinese_glyphs.h"
#include "../utils.h"
#include <math.h>

ModuleInterface* createRf433Tool() { return new Rf433Tool(); }

// Frequency sweep table for RF scan (common ISM/LPD freqs, MHz)
static const uint16_t SWEEP_FREQS[] = {315, 330, 350, 370, 390, 410, 418, 433, 440, 450};
static const uint8_t SWEEP_COUNT = 10;
#define SWEEP_DWELL_MS 3000  // listen 3 seconds per frequency

const char* Rf433Tool::getTitle() const {
    switch (state) {
        case RF_MENU:        return "RF工具箱";
        case RF_SCAN:        return "射频扫描";
        case RF_BROWSE:      return "已捕获码";
        case RF_COPY_SEND:   return "复制发送";
        case RF_CUSTOM_SEND: return "自定义发送";
        case RF_DEVDB:       return "设备库";
        case RF_TEMP_SENSOR: return "温度传感";
        default: return "RF工具箱";
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
    txFeedback = 0; selectedMask = 0; copyCursor = 0;
    rfFreq = RF433_FREQ_DFL;
    scanSweep = false; scanSweepIdx = 0; scanLastSweepMs = 0;
    scanRawActivityMs = 0;
    scanPinLowCnt = 0; scanPinPollCnt = 0;
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
    pinMode(PIN_RX433, INPUT_PULLUP);
    rcSwitch.enableReceive(digitalPinToInterrupt(PIN_RX433));
    // Wider tolerance for frequencies far from 433 MHz
    int offset = abs((int)rfFreq - 433);
    int tolerance = 60 + offset / 2;  // 60% base + extra for off-center freqs
    if (tolerance > 90) tolerance = 90;
    rcSwitch.setReceiveTolerance(tolerance);
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
        // Direct GPIO poll: SYN480R DATA pin goes LOW when carrier detected
        if (state == RF_SCAN) {
            scanPinPollCnt++;
            if (digitalRead(PIN_RX433) == LOW) scanPinLowCnt++;
        }

        // Check raw timings even when no valid protocol decoded
        unsigned int* raw = rcSwitch.getReceivedRawdata();
        bool rawActivity = (raw && raw[0] > 0);
        if (rawActivity) {
            scanRawCnt = 0;
            for (int i = 0; i < RF433_MAX_PULSES && raw[i] > 0; i++) {
                scanRaw[i] = raw[i];
                scanRawCnt++;
            }
            analyzePulses();
        }

        if (!rcSwitch.available()) {
            if (rawActivity) {
                // Show raw pulse count even without decode
                scanRawActivityMs = millis();
                displayMgr.setDirty();
            }
            return;
        }

        unsigned long val = rcSwitch.getReceivedValue();
        uint8_t bits = rcSwitch.getReceivedBitlength();
        uint8_t proto = rcSwitch.getReceivedProtocol();

        if (state == RF_SCAN) {
            scanLastVal   = val;
            scanLastBits  = bits;
            scanLastProto = proto;
            scanSignalCnt++;
            addCode(val, bits, proto);
        } else {
            tryDecodeTemp(val, bits, proto);
            addCode(val, bits, proto);
        }
        rcSwitch.resetAvailable();
        scanRawActivityMs = 0;
        displayMgr.setDirty();
    }
}

// ============================================================
// Button handling
// ============================================================
void Rf433Tool::handleButton(ButtonEvent ev) {
    switch (state) {
        case RF_MENU:
            if (ev == BTN_UP_SHORT || ev == BTN_UP_REPEAT) {
                menuCursor = (menuCursor == 0) ? 5 : menuCursor - 1;
                recomputeMenuTargets();
            }
            if (ev == BTN_DOWN_SHORT || ev == BTN_DOWN_REPEAT) {
                menuCursor = (menuCursor >= 5) ? 0 : menuCursor + 1;
                recomputeMenuTargets();
            }
            if (ev == BTN_OK_SHORT) {
                disableRX(); disableTX();
                switch (menuCursor) {
                    case 0: state = RF_SCAN; enableRX(); scanSignalCnt = 0;
                            scanSweep = false; scanSweepIdx = 0; break;
                    case 1: state = RF_BROWSE; codeCursor = codeCount > 0 ? codeCount-1 : 0; break;
                    case 2: state = RF_COPY_SEND;
                            selectedMask = 0; copyCursor = 0; txCount = 0; break;
                    case 3: state = RF_CUSTOM_SEND; enableTX();
                            txCount = 0; txFeedback = 0; break;
                    case 4: state = RF_DEVDB; dbCursor = 0; dbDetail = false; break;
                    case 5: state = RF_TEMP_SENSOR; enableRX(); tempVal = 0; tempTs = 0; break;
                }
            }
            break;

        case RF_SCAN:
            if (ev == BTN_UP_SHORT && codeCount > 0) {
                codeCursor = (codeCursor == 0) ? codeCount - 1 : codeCursor - 1;
            }
            if (ev == BTN_DOWN_SHORT && codeCount > 0) {
                codeCursor = (codeCursor >= codeCount - 1) ? 0 : codeCursor + 1;
            }
            if (ev == BTN_OK_SHORT) {
                scanSignalCnt = 0; scanLastVal = 0;
                scanRawCnt = 0; scanAvgUs = 0;
            }
            break;

        case RF_BROWSE:
            if (ev == BTN_UP_SHORT) {
                codeCursor = (codeCursor == 0) ? codeCount - 1 : codeCursor - 1;
            }
            if (ev == BTN_DOWN_SHORT) {
                codeCursor = (codeCursor >= codeCount - 1) ? 0 : codeCursor + 1;
            }
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

        case RF_COPY_SEND:
            if (codeCount == 0) break;
            if (ev == BTN_UP_SHORT) {
                copyCursor = (copyCursor == 0) ? codeCount - 1 : copyCursor - 1;
            }
            if (ev == BTN_DOWN_SHORT) {
                copyCursor = (copyCursor >= codeCount - 1) ? 0 : copyCursor + 1;
            }
            if (ev == BTN_OK_SHORT) {
                // Toggle selection
                if (selectedMask & (1 << copyCursor))
                    selectedMask &= ~(1 << copyCursor);
                else
                    selectedMask |= (1 << copyCursor);
            }
            if (ev == BTN_UP_LONG) {
                // Send all selected sequentially
                if (selectedMask) {
                    enableTX();
                    for (uint8_t i = 0; i < codeCount; i++) {
                        if (selectedMask & (1 << i)) {
                            rcSwitch.setProtocol(codes[i].protocol);
                            rcSwitch.setRepeatTransmit(3);
                            rcSwitch.send(codes[i].value, codes[i].bitLength);
                            delay(80); // gap between sends
                        }
                    }
                    txCount++;
                    txFeedback = millis();
                    disableTX();
                }
            }
            break;

        case RF_CUSTOM_SEND:
            if (ev == BTN_UP_SHORT) {
                // Cycle bit length: 12→24→32→12
                if (txBits >= 32) txBits = 12;
                else if (txBits >= 24) txBits = 32;
                else txBits = 24;
            }
            if (ev == BTN_DOWN_SHORT) {
                if (txBits <= 12) txBits = 32;
                else if (txBits <= 24) txBits = 12;
                else txBits = 24;
            }
            if (ev == BTN_UP_LONG) {
                txPulseUs += 50;
                if (txPulseUs > 2000) txPulseUs = 2000;
            }
            if (ev == BTN_DOWN_LONG) {
                if (txPulseUs > 100) txPulseUs -= 50;
                else txPulseUs = 100;
            }
            if (ev == BTN_OK_SHORT) {
                rcSwitch.setProtocol(txProto);
                rcSwitch.setPulseLength(txPulseUs);
                rcSwitch.setRepeatTransmit(5);
                rcSwitch.send(txCode, txBits);
                txCount++;
                txFeedback = millis();
            }
            if (ev == BTN_UP_DOUBLE && txProto < 7) txProto++;
            if (ev == BTN_DOWN_DOUBLE && txProto > 1) txProto--;
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
                if (ev == BTN_UP_SHORT) {
                    dbCursor = (dbCursor == 0) ? RF433_DB_COUNT - 1 : dbCursor - 1;
                }
                if (ev == BTN_DOWN_SHORT) {
                    dbCursor = (dbCursor >= RF433_DB_COUNT - 1) ? 0 : dbCursor + 1;
                }
                if (ev == BTN_OK_SHORT) dbDetail = true;
            }
            break;

        case RF_TEMP_SENSOR:
            if (ev == BTN_OK_SHORT) { tempVal = 0; humidity = 0; tempTs = 0; }
            break;
    }
    displayMgr.setDirty();
}

bool Rf433Tool::handleBack() {
    if (state != RF_MENU) {
        disableRX();
        disableTX();
        state = RF_MENU;
        menuCursor = 0;
        recomputeMenuTargets();
        displayMgr.setDirty();
        return true;
    }
    return false;
}

// ============================================================
// Draw dispatch
// ============================================================
void Rf433Tool::draw(U8G2& u8g2) {
    switch (state) {
        case RF_MENU:        drawMenu(u8g2); break;
        case RF_SCAN:        drawScan(u8g2); break;
        case RF_BROWSE:      drawBrowse(u8g2); break;
        case RF_COPY_SEND:   drawCopySend(u8g2); break;
        case RF_CUSTOM_SEND: drawCustomSend(u8g2); break;
        case RF_DEVDB:       drawDevDb(u8g2); break;
        case RF_TEMP_SENSOR: drawTemp(u8g2); break;
    }
}

// ============================================================
// Draw — RF_MENU
// ============================================================
void Rf433Tool::drawMenu(U8G2& u8g2) {
    static const char* itemCN[6] = {
        "射频扫描",
        "已捕获码",
        "复制发送",
        "自定义发送",
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

    // 2. Scrollbar (animated)
    u8g2.drawVLine(MENU_SCROLLBAR_X + 1, 0, OLED_HEIGHT);
    float sbH = OLED_HEIGHT * 3.0f / 6.0f;
    float maxScroll = (6 - 3) * MENU_ROW_HEIGHT;
    float frac = (maxScroll > 0) ? mScrollY / maxScroll : 0;
    float sbY = (OLED_HEIGHT - sbH) * frac;
    u8g2.drawBox(MENU_SCROLLBAR_X, (int)sbY, MENU_SCROLLBAR_W, max(4, (int)sbH));

    // 3. XOR selection box (animated)
    int tw = cnStrWidth(itemCN[menuCursor]);
    mSelWTarget = tw + MENU_SEL_PAD_X * 2;
    int bw = max(20, (int)mSelW);
    u8g2.setDrawColor(2);
    u8g2.drawRBox(MENU_TEXT_X - MENU_SEL_PAD_X, (int)mSelY, bw, MENU_ROW_HEIGHT - 1, MENU_SEL_RADIUS);
    u8g2.setDrawColor(1);

    // 4. Footer
    u8g2.setFont(FONT_SMALL);
}

// ============================================================
// Draw — RF_SCAN
// ============================================================
void Rf433Tool::drawScan(U8G2& u8g2) {
    // Top line: fixed frequency + signal count
    u8g2.setFont(FONT_DATA);
    char top[24];
    snprintf(top, sizeof(top), "433.92MHz  %lu sigs", scanSignalCnt);
    u8g2.drawStr(0, 9, top);

    // Show raw pulse activity indicator (receiver is hearing something)
    bool rawRecent = (scanRawActivityMs && millis() - scanRawActivityMs < 1000);

    // Live GPIO pin state: LOW = carrier detected by SYN480R
    u8g2.setFont(FONT_BODY);
    bool pinLo = (digitalRead(PIN_RX433) == LOW);
    char pinLine[32];
    if (scanPinPollCnt > 0) {
        uint32_t pct = (uint32_t)((uint64_t)scanPinLowCnt * 100 / scanPinPollCnt);
        snprintf(pinLine, sizeof(pinLine), "PIN:%s L=%lu/%lu %lu%%",
                 pinLo ? "L" : "H", scanPinLowCnt, scanPinPollCnt, pct);
    } else {
        snprintf(pinLine, sizeof(pinLine), "PIN:%s", pinLo ? "L" : "H");
    }
    u8g2.drawStr(0, 20, pinLine);

    if (scanSignalCnt == 0) {
        drawCN(u8g2, 20, 38, "监听中...");
        if (rawRecent) {
            u8g2.setFont(FONT_BODY);
            char rbuf[24];
            snprintf(rbuf, sizeof(rbuf), "RAW:%dpls %dus", scanRawCnt, scanAvgUs);
            u8g2.drawStr(10, 50, rbuf);
        }
        return;
    }

    // Show recent captured codes as a list (last 3)
    u8g2.setFont(FONT_BODY);
    for (uint8_t i = 0; i < 3 && i < codeCount; i++) {
        uint8_t y = 30 + i * 14;
        uint8_t idx = codeCount - 1 - i;
        char buf[40];
        snprintf(buf, sizeof(buf), "#%d: %lu %db %s",
                 idx + 1, codes[idx].value, codes[idx].bitLength,
                 protoName(codes[idx].protocol));
        u8g2.drawStr(2, y, buf);
    }
    if (rawRecent && scanSignalCnt > 0) {
        u8g2.setFont(FONT_SMALL);
        char rbuf[20];
        snprintf(rbuf, sizeof(rbuf), "raw:%dpls", scanRawCnt);
        u8g2.drawStr(70, 63, rbuf);
    }
}

// ============================================================
// Draw — RF_BROWSE
// ============================================================
void Rf433Tool::drawBrowse(U8G2& u8g2) {
    if (codeCount == 0) {
        drawCN(u8g2, 10, 38, "无捕获信号");
        return;
    }

    // Show list of captured codes
    for (uint8_t i = 0; i < 3 && i < codeCount; i++) {
        uint8_t y = 16 + i * 16;
        uint8_t idx = codeCount - 1 - i;
        if (idx == codeCursor) {
            u8g2.drawBox(0, y - 12, OLED_WIDTH, 14);
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

    // Detail for selected code
    uint8_t ci = codeCursor;
    u8g2.setFont(FONT_BODY);
    char dbuf[32];
    snprintf(dbuf, sizeof(dbuf), "DEC:%lu B:%d P:%d",
             codes[ci].value, codes[ci].bitLength, codes[ci].protocol);
    u8g2.drawStr(0, 63, dbuf);
}

// ============================================================
// Draw — RF_COPY_SEND (multi-select → sequential send)
// ============================================================
void Rf433Tool::drawCopySend(U8G2& u8g2) {
    if (codeCount == 0) {
        drawCN(u8g2, 10, 38, "无捕获信号");
        return;
    }

    // List codes with checkboxes
    for (uint8_t i = 0; i < 3 && i < codeCount; i++) {
        uint8_t y = 16 + i * 16;
        uint8_t idx = codeCount - 1 - i;
        if (i == copyCursor) {
            u8g2.drawBox(0, y - 12, OLED_WIDTH, 14);
            u8g2.setDrawColor(0);
        }

        // Checkbox: [*] or [ ]
        bool sel = selectedMask & (1 << idx);
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(2, y, sel ? "[*]" : "[ ]");

        char buf[32];
        snprintf(buf, sizeof(buf), "%lu %db %s", codes[idx].value,
                 codes[idx].bitLength, protoName(codes[idx].protocol));
        u8g2.drawStr(22, y, buf);

        if (i == copyCursor) u8g2.setDrawColor(1);
    }

    // TX feedback flash
    if (txFeedback && millis() - txFeedback < 500) {
        u8g2.setFont(FONT_BIG);
        u8g2.drawStr(30, 55, "Sent!");
    }
}

// ============================================================
// Draw — RF_CUSTOM_SEND (any Hz, ±1 short, ±10 long, OK=send)
// ============================================================
void Rf433Tool::drawCustomSend(U8G2& u8g2) {
    // Fixed frequency (crystal-locked)
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(2, 14, "Freq:433.92MHz(固定)");

    // Code info
    char buf[32];
    snprintf(buf, sizeof(buf), "Code:%lu B:%d", txCode, txBits);
    u8g2.drawStr(2, 28, buf);
    snprintf(buf, sizeof(buf), "Proto:%s P:%dus", protoName(txProto), txPulseUs);
    u8g2.drawStr(2, 38, buf);

    // Controls
    u8g2.setFont(FONT_SMALL);
    u8g2.drawStr(0, 52, "Dbl+/-:proto  OK:send");

    // TX feedback
    if (txFeedback && millis() - txFeedback < 500) {
        u8g2.setFont(FONT_BIG);
        u8g2.drawStr(30, 58, "Sent!");
    }
}

// ============================================================
// Draw — RF_DEVDB
// ============================================================
void Rf433Tool::drawDevDb(U8G2& u8g2) {
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
        u8g2.setFont(FONT_DATA);
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
    }
}

// ============================================================
// Draw — RF_TEMP_SENSOR
// ============================================================
void Rf433Tool::drawTemp(U8G2& u8g2) {
    if (tempVal == 0 && tempTs == 0) {
        drawCN(u8g2, 10, 38, "监听中...");
    } else {
        u8g2.setFont(FONT_BIG);
        char buf[32];
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
}
