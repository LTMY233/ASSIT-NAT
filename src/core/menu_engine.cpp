#include "menu_engine.h"
#include "config.h"
#include "modules/module_registry.h"
#include "display_mgr.h"
#include "icons.h"
#include "chinese_glyphs.h"
#include <math.h>

MenuEngine menuEngine;

// ============================================================
// Abbreviation & icon mapping
// ============================================================
const char* MenuEngine::getAbbrForModule(uint8_t id) {
    switch (id) {
        // Cat 0 - Network Recon
        case 0:  return "WiFi扫描";    case 1:  return "热力图";
        case 2:  return "探针嗅探";    case 3:  return "主机发现";
        case 4:  return "网关监控";    case 5:  return "端口扫描";
        case 6:  return "路由追踪";    case 7:  return "网络统计";
        case 8:  return "DHCP发现";    case 9:  return "mDNS枚举";
        case 10: return "UPnP发现";
        case 11: return "DNS查询";     case 12: return "HTTP客户端";
        case 13: return "Ping监控";    case 14: return "NTP查询";
        case 15: return "网络唤醒";
        // Cat 1 - RF Analysis
        case 20: return "信标解码";    case 21: return "帧计数器";
        case 22: return "距离估算";    case 23: return "信道质量";
        case 24: return "信号直方图";
        case 25: return "频谱视图";    case 26: return "信道跳频";
        case 27: return "信号监控";    case 28: return "射频噪声";
        // Cat 2 - WiFi Attack
        case 40: return "WiFi攻击中心";
        case 48: return "WPS PIN";     case 49: return "SSL检查";
        // Cat 3 - Crypto
        case 30: return "动态口令";    case 31: return "密码生成";
        case 32: return "厂商查询";
        case 33: return "Base64编码";  case 34: return "十六进制";
        case 35: return "哈希计算";    case 36: return "异或加密";
        // Cat 4 - Hardware
        case 50: return "逻辑探针";    case 51: return "PWM发生器";
        case 52: return "ADC电压";     case 53: return "I2C扫描";
        case 54: return "GPIO控制";    case 55: return "串口监视";
        case 56: return "SPI扫描";     case 57: return "频率发生器";
        // Cat 5 - Network Diag
        case 60: return "网关发现";    case 61: return "多网关检测";
        case 62: return "信道切换";    case 63: return "信号漂移";
        case 64: return "广播风暴";    case 65: return "服务发现";
        case 66: return "WiFi日志";    case 67: return "网络快照";
        case 68: return "网速测试";    case 69: return "子网计算";
        // Cat 6 - 433MHz
        case 70: return "433MHz工具箱";
        // Cat 7 - System
        case 80: return "系统信息";    case 81: return "文件管理";
        case 82: return "屏幕测试";    case 83: return "按键测试";
        case 84: return "电池电压";    case 85: return "休眠定时";
        case 86: return "自动配置";    case 87: return "LED测试";
        case 88: return "系统设置";
        default: return "???";
    }
}

const unsigned char* MenuEngine::getIconForModule(uint8_t id) {
    switch (id) {
        case 0:  return icon_wifi_scanner; case 1:  return icon_channel;
        case 2:  return icon_sniffer;      case 3:  return icon_host;
        case 4:  return icon_gateway;      case 5:  return icon_port;
        case 6:  return icon_traceroute;   case 7:  return icon_net_stats;
        case 8:  return icon_dhcp;         case 9:  return icon_mdns;
        case 10: return icon_upnp;
        case 20: return icon_beacon;       case 21: return icon_frame;
        case 22: return icon_distance;     case 23: return icon_quality;
        case 24: return icon_histogram;
        case 40: return icon_deauth;
        case 30: return icon_totp;         case 31: return icon_password;
        case 32: return icon_mac;
        case 50: return icon_probe;        case 51: return icon_pwm;
        case 52: return icon_adc;          case 53: return icon_i2c;
        case 60: return icon_gw_disc;      case 61: return icon_alert;
        case 62: return icon_ch_switch;    case 63: return icon_drift;
        case 64: return icon_storm;        case 65: return icon_service;
        case 66: return icon_log;          case 67: return icon_snapshot;
        default: return nullptr;
    }
}

const unsigned char* MenuEngine::getCatIcon(uint8_t cat) {
    switch (cat) {
        case 0: return icon_wifi_scanner;
        case 1: return icon_beacon;
        case 2: return icon_deauth;
        case 3: return icon_totp;
        case 4: return icon_i2c;
        case 5: return icon_snapshot;
        case 6: return icon_deauth;
        case 7: return icon_tests;
        default: return nullptr;
    }
}

const char* MenuEngine::getCategoryName(uint8_t catId) {
    switch (catId) {
        case 0: return "网络侦察";
        case 1: return "射频分析";
        case 2: return "WiFi攻击";
        case 3: return "加密工具";
        case 4: return "硬件诊断";
        case 5: return "网络诊断";
        case 6: return "433MHz射频";
        case 7: return "系统工具";
        default: return nullptr;
    }
}

// ============================================================
// Init & build
// ============================================================
void MenuEngine::init() {
    state = STATE_MENU_MAIN;
    pageIndex = 0;
    cursorIndex = 0;
    lastUpdateMs = millis();

    for (uint8_t i = 0; i < MENU_MAX_PAGES; i++) {
        pages[i].title = nullptr;
        pages[i].itemCount = 0;
    }

    scrollY = scrollYTarget = 0;
    selY = selYTarget = 0;
    selW = selWTarget = 0;
    sbY = sbYTarget = 0;
    sbH = sbHTarget = 0;
}

void MenuEngine::buildMenu() {
    for (uint8_t c = 0; c < 8; c++) {
        pages[c + 1].title = getCategoryName(c);
        pages[c + 1].itemCount = 0;
    }

    for (uint8_t i = 0; i < moduleRegistry.getCount(); i++) {
        ModuleInterface* mod = moduleRegistry.getByIndex(i);
        if (!mod) continue;
        uint8_t cat = mod->getCategory();
        if (cat >= 8) continue;
        MenuPage& page = pages[cat + 1];
        if (page.itemCount >= MENU_MAX_ITEMS) continue;

        MenuItem& item = page.items[page.itemCount];
        item.label = getAbbrForModule(mod->getId());
        item.toolId = mod->getId();
        item.icon = mod->getIcon();
        if (!item.icon) item.icon = getIconForModule(mod->getId());
        page.itemCount++;
    }

    pages[0].title = "ASSIT-NAT";
    pages[0].itemCount = 0;
    for (uint8_t c = 0; c < 8; c++) {
        if (pages[c + 1].itemCount > 0) {
            MenuItem& item = pages[0].items[pages[0].itemCount];
            item.label = getCategoryName(c);
            item.toolId = 0xFF;
            item.icon = getCatIcon(c);
            pages[0].itemCount++;
        }
    }

    pageIndex = 0;
    cursorIndex = 0;
    snapToTargets();
    lastUpdateMs = millis();
}

// ============================================================
// Target computation
// ============================================================
void MenuEngine::recomputeTargets() {
    MenuPage& page = pages[pageIndex];
    uint8_t n = page.itemCount;
    if (n == 0) return;
    if (cursorIndex >= n) cursorIndex = n - 1;

    // Scroll target: keep cursor visible
    if (n <= MENU_VISIBLE) {
        scrollYTarget = 0;
    } else if (cursorIndex < MENU_VISIBLE) {
        // Cursor near top -> list top
        scrollYTarget = 0;
    } else if (cursorIndex >= n - 1) {
        // Cursor at end -> list bottom
        scrollYTarget = (float)(n - MENU_VISIBLE) * MENU_ROW_HEIGHT;
    } else {
        // Cursor in middle -> keep at visible row 2 (idx 1)
        scrollYTarget = (float)(cursorIndex - 1) * MENU_ROW_HEIGHT;
    }

    // Sel box Y = cursor row screen position
    selYTarget = (float)cursorIndex * MENU_ROW_HEIGHT - scrollYTarget + MENU_TOP_PAD;

    // Scrollbar
    float contentH = (float)OLED_HEIGHT;
    if (n > MENU_VISIBLE) {
        sbHTarget = contentH * MENU_VISIBLE / n;
        float maxSbY = contentH - sbHTarget;
        sbYTarget = maxSbY * cursorIndex / max(1, n - 1);
    } else {
        sbHTarget = contentH;
        sbYTarget = 0;
    }
}

void MenuEngine::snapToTargets() {
    recomputeTargets();
    scrollY = scrollYTarget;
    selY = selYTarget;
    sbY = sbYTarget;
    sbH = sbHTarget;
}

bool MenuEngine::isAnimating() const {
    return fabsf(scrollYTarget - scrollY) > ANIM_SNAP_THRESH
        || fabsf(selYTarget - selY)     > ANIM_SNAP_THRESH
        || fabsf(selWTarget - selW)     > ANIM_SNAP_THRESH
        || fabsf(sbYTarget - sbY)       > ANIM_SNAP_THRESH
        || fabsf(sbHTarget - sbH)       > ANIM_SNAP_THRESH;
}

// ============================================================
// Frame update - time-driven exp decay
// ============================================================
void MenuEngine::update() {
    if (state != STATE_MENU_MAIN && state != STATE_MENU_CATEGORY) return;

    uint32_t now = millis();
    float dt = (now - lastUpdateMs) / 1000.0f;
    lastUpdateMs = now;
    if (dt <= 0.0f || dt > ANIM_DT_MAX) dt = ANIM_DT_MAX;

    float t = 1.0f - expf(-ANIM_SPEED * dt);

    scrollY += (scrollYTarget - scrollY) * t;
    selY    += (selYTarget - selY) * t;
    selW    += (selWTarget - selW) * t;
    sbY     += (sbYTarget - sbY) * t;
    sbH     += (sbHTarget - sbH) * t;

    // Snap when close to target
    if (fabsf(scrollYTarget - scrollY) < ANIM_SNAP_THRESH) scrollY = scrollYTarget;
    if (fabsf(selYTarget - selY)     < ANIM_SNAP_THRESH) selY    = selYTarget;
    if (fabsf(selWTarget - selW)     < ANIM_SNAP_THRESH) selW    = selWTarget;
    if (fabsf(sbYTarget - sbY)       < ANIM_SNAP_THRESH) sbY     = sbYTarget;
    if (fabsf(sbHTarget - sbH)       < ANIM_SNAP_THRESH) sbH     = sbHTarget;

    if (isAnimating()) displayMgr.setDirty();
}

// ============================================================
// Render - Chinese text via per-char drawCN(), ASCII via u8g2
// ============================================================
void MenuEngine::draw(U8G2& u8g2) {
    MenuPage& page = pages[pageIndex];
    if (page.itemCount == 0) return;

    // Selected item width (update anim target)
    const char* selLabel = page.items[cursorIndex].label;
    uint8_t tw = cnStrWidth(selLabel);
    if (tw == 0) {
        u8g2.setFont(FONT_MENU);
        tw = u8g2.getStrWidth(selLabel);  // ASCII fallback
    }
    selWTarget = tw + MENU_SEL_PAD_X * 2;

    const int cnBaseOff = MENU_ROW_HEIGHT - 2;

    // 1. Text items (list scrolls together)
    for (uint8_t i = 0; i < page.itemCount; i++) {
        const char* label = page.items[i].label;
        int y = (int)(i * MENU_ROW_HEIGHT - scrollY + MENU_TOP_PAD + cnBaseOff);
        if (y < -14 || y > OLED_HEIGHT + 16) continue;

        // Chinese or ASCII?
        if (cnStrWidth(label) > 0) {
            drawCN(u8g2, MENU_TEXT_X, y, label);
        } else {
            u8g2.setFont(FONT_MENU);
            u8g2.drawStr(MENU_TEXT_X, y - 2, label);
        }
    }

    // 2. Scrollbar
    if (page.itemCount > MENU_VISIBLE) {
        u8g2.drawVLine(MENU_SCROLLBAR_X + 1, 0, OLED_HEIGHT);
        int th = max(4, (int)sbH);
        u8g2.drawBox(MENU_SCROLLBAR_X, (int)sbY, MENU_SCROLLBAR_W, th);
    }

    // 3. XOR selection box
    u8g2.setDrawColor(2);
    int bx = MENU_TEXT_X - MENU_SEL_PAD_X;
    int by = (int)selY;
    int bw = max(20, (int)selW);
    int bh = MENU_ROW_HEIGHT;
    u8g2.drawRBox(bx, by, bw, bh - 1, MENU_SEL_RADIUS);
    u8g2.setDrawColor(1);
}

// ============================================================
// Navigation
// ============================================================
void MenuEngine::navigateUp() {
    MenuPage& page = pages[pageIndex];
    if (page.itemCount == 0) return;
    if (cursorIndex > 0) cursorIndex--;
    else cursorIndex = page.itemCount - 1;
    recomputeTargets();
    displayMgr.setDirty();
}

void MenuEngine::navigateDown() {
    MenuPage& page = pages[pageIndex];
    if (page.itemCount == 0) return;
    if (cursorIndex < page.itemCount - 1) cursorIndex++;
    else cursorIndex = 0;
    recomputeTargets();
    displayMgr.setDirty();
}

void MenuEngine::navigateConfirm() {
    MenuPage& page = pages[pageIndex];
    if (page.itemCount == 0) return;

    MenuItem& item = page.items[cursorIndex];

    // Blink confirm
    U8G2& u8g2 = displayMgr.getU8g2();
    u8g2.setDrawColor(2);
    u8g2.drawBox(0, 0, OLED_WIDTH, OLED_HEIGHT);
    u8g2.sendBuffer();
    delay(60);
    u8g2.setDrawColor(1);
    draw(u8g2);
    u8g2.sendBuffer();

    if (item.toolId != 0xFF) {
        ModuleInterface* mod = moduleRegistry.launch(item.toolId);
        if (mod) {
            state = STATE_TOOL_RUNNING;
        } else {
            u8g2.clearBuffer();
            u8g2.setFont(FONT_BODY);
            u8g2.drawStr(20, 32, "Launch FAIL!");
            u8g2.sendBuffer();
            delay(800);
        }
        displayMgr.setDirty();
        return;
    }

    // Enter category sub-page
    if (pageIndex == 0 && cursorIndex < pages[0].itemCount) {
        pageIndex = cursorIndex + 1;
        cursorIndex = 0;
        state = STATE_MENU_CATEGORY;
        snapToTargets();
        displayMgr.setDirty();
    }
}

void MenuEngine::navigateBack() {
    // Active module -> exit module
    if (moduleRegistry.active()) {
        moduleRegistry.exitCurrent();
        state = (pageIndex == 0) ? STATE_MENU_MAIN : STATE_MENU_CATEGORY;
        displayMgr.setDirty();
        return;
    }
    // Category subpage -> back to main
    if (state == STATE_MENU_CATEGORY) {
        pageIndex = 0;
        state = STATE_MENU_MAIN;
        snapToTargets();
        displayMgr.setDirty();
    }
}

uint8_t MenuEngine::getSelectedToolId() const {
    if (pageIndex == 0) return 0xFF;
    const MenuPage& page = pages[pageIndex];
    if (cursorIndex < page.itemCount)
        return page.items[cursorIndex].toolId;
    return 0xFF;
}

// ============================================================
// Button handling
// ============================================================
void MenuEngine::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_UP_SHORT:   navigateUp();      break;
        case BTN_DOWN_SHORT: navigateDown();    break;
        case BTN_OK_SHORT:   navigateConfirm(); break;
        case BTN_OK_LONG:    navigateBack();    break;
        case BTN_OK_DOUBLE:
            if (state == STATE_TOOL_RUNNING) navigateBack();
            break;
        case BTN_UP_LONG:
            if (pages[pageIndex].itemCount > 0) {
                if (cursorIndex >= MENU_VISIBLE) cursorIndex -= MENU_VISIBLE;
                else cursorIndex = 0;
                recomputeTargets();
                displayMgr.setDirty();
            }
            break;
        case BTN_DOWN_LONG:
            if (pages[pageIndex].itemCount > 0) {
                if (cursorIndex + MENU_VISIBLE < pages[pageIndex].itemCount)
                    cursorIndex += MENU_VISIBLE;
                else cursorIndex = pages[pageIndex].itemCount - 1;
                recomputeTargets();
                displayMgr.setDirty();
            }
            break;
        case BTN_UP_DOUBLE:
            if (pages[pageIndex].itemCount > 0) {
                cursorIndex = 0;
                recomputeTargets();
                displayMgr.setDirty();
            }
            break;
        case BTN_DOWN_DOUBLE:
            if (pages[pageIndex].itemCount > 0) {
                cursorIndex = pages[pageIndex].itemCount - 1;
                recomputeTargets();
                displayMgr.setDirty();
            }
            break;
        case BTN_UP_REPEAT:   navigateUp();      break;
        case BTN_DOWN_REPEAT: navigateDown();    break;
        default: break;
    }
}
