#!/usr/bin/env python3
"""Generate XBM bitmaps for Chinese menu strings used in ESP8266 OLED.
Each menu string is pre-rendered as an XBM, stored in PROGMEM,
and drawn with u8g2.drawXBM() for fast rendering."""

from PIL import Image, ImageFont, ImageDraw
import os

# ============================================================
# All translatable strings
# ============================================================
# (string_id, chinese_text)
MENU_STRINGS = [
    # Categories
    ("CAT_NETWORK_RECON", "网络侦察"),
    ("CAT_RF_ANALYSIS", "射频分析"),
    ("CAT_WIFI_ATTACK", "WiFi攻击"),
    ("CAT_CRYPTO", "加密工具"),
    ("CAT_HW_DIAG", "硬件诊断"),
    ("CAT_NET_DIAG", "网络诊断"),
    ("CAT_433MHZ", "433MHz射频"),
    ("CAT_SYSTEM", "系统工具"),

    # Module names — Cat 0: Network Recon
    ("MOD_WIFI_SCAN", "WiFi扫描"),
    ("MOD_HEATMAP", "热力图"),
    ("MOD_PROBE_SNIFF", "探针嗅探"),
    ("MOD_HOST_DISC", "主机发现"),
    ("MOD_GW_MONITOR", "网关监控"),
    ("MOD_PORT_SCAN", "端口扫描"),
    ("MOD_TRACEROUTE", "路由追踪"),
    ("MOD_NET_STATS", "网络统计"),
    ("MOD_DHCP_FIND", "DHCP发现"),
    ("MOD_MDNS_ENUM", "mDNS枚举"),
    ("MOD_UPNP_FIND", "UPnP发现"),
    ("MOD_DNS_LOOKUP", "DNS查询"),
    ("MOD_HTTP_CLIENT", "HTTP客户端"),
    ("MOD_PING_MON", "Ping监控"),
    ("MOD_NTP_QUERY", "NTP查询"),
    ("MOD_WAKE_ON_LAN", "网络唤醒"),

    # Module names — Cat 1: RF Analysis
    ("MOD_BEACON_DEC", "信标解码"),
    ("MOD_FRAME_CNT", "帧计数器"),
    ("MOD_RSSI_DIST", "距离估算"),
    ("MOD_CH_QUALITY", "信道质量"),
    ("MOD_RSSI_HIST", "信号直方图"),
    ("MOD_SPECTRUM", "频谱视图"),
    ("MOD_CH_HOPPER", "信道跳频"),
    ("MOD_SIG_MON", "信号监控"),
    ("MOD_RF_NOISE", "射频噪声"),

    # Module names — Cat 2: WiFi Attack
    ("MOD_WIFI_ATTACK", "WiFi攻击中心"),

    # WifiAttack sub-menu
    ("WA_SCAN_TARGETS", "扫描目标"),
    ("WA_SELECT_TARGET", "选择目标"),
    ("WA_ATTACK_MODE", "攻击模式"),
    ("WA_CONFIGURE", "配置参数"),
    ("WA_LAUNCH", "开始攻击"),
    ("WA_DEV_TESTS", "设备测试"),

    # WifiAttack attack types
    ("ATK_DEAUTH", "解除认证"),
    ("ATK_BEACON", "信标洪水"),
    ("ATK_PMKID", "PMKID捕获"),
    ("ATK_HANDSHAKE", "握手捕获"),
    ("ATK_ROGUE", "伪基站"),
    ("ATK_KARMA", "Karma攻击"),
    ("ATK_EVIL_TWIN", "钓鱼热点"),

    # Module names — Cat 3: Crypto
    ("MOD_TOTP", "动态口令"),
    ("MOD_PASSWD", "密码生成"),
    ("MOD_MAC_OUI", "厂商查询"),
    ("MOD_BASE64", "Base64编码"),
    ("MOD_HEX", "十六进制"),
    ("MOD_HASH", "哈希计算"),
    ("MOD_XOR", "异或加密"),

    # Module names — Cat 4: Hardware
    ("MOD_LOGIC_PROBE", "逻辑探针"),
    ("MOD_PWM", "PWM发生器"),
    ("MOD_ADC", "ADC电压"),
    ("MOD_I2C", "I2C扫描"),
    ("MOD_GPIO", "GPIO控制"),
    ("MOD_UART", "串口监视"),
    ("MOD_SPI", "SPI扫描"),
    ("MOD_FREQ", "频率发生器"),

    # Module names — Cat 5: Network Diag
    ("MOD_GW_FIND", "网关发现"),
    ("MOD_MULTI_GW", "多网关检测"),
    ("MOD_CH_SWITCH", "信道切换"),
    ("MOD_RSSI_DRIFT", "信号漂移"),
    ("MOD_BCAST_STORM", "广播风暴"),
    ("MOD_SVC_DISC", "服务发现"),
    ("MOD_WIFI_LOG", "WiFi日志"),
    ("MOD_SNAPSHOT", "网络快照"),
    ("MOD_SPEED_TEST", "网速测试"),
    ("MOD_SUBNET", "子网计算"),

    # Module names — Cat 6: 433MHz
    ("MOD_433MHZ", "433MHz工具箱"),

    # Rf433Tool sub-menu
    ("RF_SCAN_433", "扫描433"),
    ("RF_BROWSE", "已捕获码"),
    ("RF_SEND", "发送433"),
    ("RF_RAW_EDIT", "原始编辑"),
    ("RF_DEV_DB", "设备库"),
    ("RF_TEMP_SENSOR", "温度传感"),

    # Module names — Cat 7: System
    ("MOD_SYS_INFO", "系统信息"),
    ("MOD_FILE_MGR", "文件管理"),
    ("MOD_SCR_TEST", "屏幕测试"),
    ("MOD_BTN_TEST", "按键测试"),
    ("MOD_BATTERY", "电池电压"),
    ("MOD_SLEEP", "休眠定时"),
    ("MOD_AUTO_CFG", "自动配置"),
    ("MOD_LED_TEST", "LED测试"),
    ("MOD_SETTINGS", "系统设置"),

    # Common UI
    ("UI_OK", "确定"),
    ("UI_BACK", "返回"),
    ("UI_CANCEL", "取消"),
    ("UI_SCANNING", "扫描中"),
    ("UI_DONE", "完成"),
    ("UI_STOP_ANYKEY", "按任意键停止"),
    ("UI_ATTACKING", "正在攻击"),
    ("UI_CONTINUOUS", "连续"),
    ("UI_SEND", "发送"),
    ("UI_MAIN_MENU", "主菜单"),

    # Sub-menu headers & status
    ("UI_CONFIRM_ATK", "确认攻击"),
    ("UI_ATK_DONE", "攻击完成"),
    ("UI_LISTENING", "监听中..."),
    ("UI_SCAN_WIFI", "扫描WiFi..."),
    ("UI_SCAN_433M", "扫描433MHz..."),
    ("UI_NO_CAPTURES", "无捕获信号"),
    ("UI_TARGET", "目标"),
    ("UI_UNLIMITED", "无限制"),
    ("UI_NO_CFG", "无需配置"),
    ("UI_OK_START", "按确定开始"),
    ("UI_DEAUTH", "解除认证中"),
    ("UI_BEACON_FLD", "信标洪水中"),
    ("UI_PMKID_CAP", "PMKID捕获中"),
    ("UI_HANDSHAKE_CAP", "握手捕获中"),
    ("UI_ROGUE_AP", "伪基站运行中"),
    ("UI_KARMA_ATK", "Karma攻击中"),
    ("UI_EVIL_TWIN", "钓鱼热点运行中"),
    ("UI_PROBE_RESP", "探针响应中"),
    ("UI_DEAUTH_CNT", "发包数"),
    ("UI_DURATION", "持续时间"),

    # Keep "ASSIT-NAT" as-is (brand name)
]

FONT_PATH = "C:/Windows/Fonts/simsun.ttc"
FONT_SIZE = 13
OUTPUT_DIR = "src"

def render_xbm(text, font, name):
    """Render text as an XBM bitmap."""
    # Get text bounding box
    bbox = font.getbbox(text)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1] + 2  # +2 for descent

    if w < 1: w = 1
    if h < 1: h = 1

    # Create image
    img = Image.new('1', (w, h), 0)
    draw = ImageDraw.Draw(img)
    draw.text((-bbox[0], -bbox[1] + 1), text, font=font, fill=1)

    # Convert to XBM format
    xbm_w = w
    xbm_h = h

    lines = []
    row_bytes = (w + 7) // 8
    for y in range(h):
        for bx in range(row_bytes):
            val = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < w and img.getpixel((x, y)):
                    val |= (1 << (7 - bit))  # XBM: MSB = leftmost pixel
            lines.append(f"0x{val:02X}")

    return w, h, lines

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    font = ImageFont.truetype(FONT_PATH, FONT_SIZE)

    # Render all strings
    xbms = {}
    for sid, text in MENU_STRINGS:
        w, h, data = render_xbm(text, font, sid)
        xbms[sid] = (w, h, data)
        print(f"  {sid}: {text} -> {w}x{h} ({len(data)} bytes)")

    # Write header
    with open(f"{OUTPUT_DIR}/chinese_xbm.h", 'w', encoding='utf-8') as f:
        f.write('''#pragma once
#include <Arduino.h>
#include <U8g2lib.h>

// Chinese XBM bitmaps for ESP8266 OLED menu
// Generated by gen_chinese_font.py

struct ChineseXBM {
    const char* id;         // offset 0 (4B, aligned)
    const uint8_t* data;    // offset 4 (4B, aligned)
    uint8_t w, h;           // offset 8,9 (1B each, no alignment needed)
};

// Lookup and draw
void drawCN(U8G2& u8g2, uint8_t x, uint8_t y, const char* id);
uint8_t cnWidth(const char* id);

''')

        # Declare all XBM data arrays
        for sid, (w, h, _) in xbms.items():
            f.write(f'extern const uint8_t cn_{sid}[] PROGMEM;\n')

        f.write(f'\n#define CN_FONT_COUNT {len(xbms)}\n')
        f.write('extern const ChineseXBM cn_table[] PROGMEM;\n')

    # Write CPP with data
    with open(f"{OUTPUT_DIR}/chinese_xbm.cpp", 'w', encoding='utf-8') as f:
        f.write('#include "chinese_xbm.h"\n\n')

        # Write XBM data for each string (raw XBM pixels, no prefix)
        for sid, (w, h, data) in xbms.items():
            f.write(f'// {sid}  ({w}x{h})\n')
            f.write(f'const uint8_t cn_{sid}[] PROGMEM = {{\n')
            for i in range(0, len(data), 16):
                chunk = data[i:i+16]
                f.write('  ' + ','.join(chunk) + ',\n')
            f.write('};\n\n')

        # Write lookup table
        f.write(f'const ChineseXBM cn_table[] PROGMEM = {{\n')
        for sid, (w, h, _) in xbms.items():
            f.write(f'  {{"{sid}", cn_{sid}, {w}, {h}}},\n')
        f.write('};\n\n')

        # Write draw function with proper pgm_read_* access
        f.write('''
// Packed struct ensures correct PROGMEM layout (no padding).
// We use pgm_read_* to safely read from flash on all architectures.

static const ChineseXBM* cn_find(const char* id) {
  for (uint8_t i = 0; i < CN_FONT_COUNT; i++) {
    const ChineseXBM* p = &cn_table[i];
    const char* sid = (const char*)pgm_read_ptr(&p->id);
    if (strcmp_P(id, sid) == 0) return p;
  }
  return nullptr;
}

void drawCN(U8G2& u8g2, uint8_t x, uint8_t y, const char* id) {
  const ChineseXBM* p = cn_find(id);
  if (!p) return;
  uint8_t w = pgm_read_byte(&p->w);
  uint8_t h = pgm_read_byte(&p->h);
  const uint8_t* data = (const uint8_t*)pgm_read_ptr(&p->data);
  u8g2.drawXBM(x, y - h, w, h, data);
}

uint8_t cnWidth(const char* id) {
  const ChineseXBM* p = cn_find(id);
  if (!p) return 0;
  return pgm_read_byte(&p->w);
}
''')

    total_bytes = sum(len(d) + 2 for _, _, d in xbms.values())
    print(f"\nGenerated {len(xbms)} XBM strings")
    print(f"Total XBM data: {total_bytes} bytes")
    print(f"Output: {OUTPUT_DIR}/chinese_xbm.h, {OUTPUT_DIR}/chinese_xbm.cpp")

if __name__ == '__main__':
    main()
