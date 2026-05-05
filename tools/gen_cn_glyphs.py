#!/usr/bin/env python3
"""Generate per-character XBM glyphs for Chinese text on ESP8266 OLED.
Each unique Chinese character is rendered as a small XBM bitmap (~12x12px).
Strings are drawn character-by-character via drawXBM().

This avoids the large-XBM boot-loop issue and the complexity of U8g2 font format.
"""

from PIL import Image, ImageFont, ImageDraw
import os, math

FONT_PATH = "C:/Windows/Fonts/simsun.ttc"
FONT_SIZE = 13
OUTPUT_DIR = "src"

# All strings that need Chinese rendering
MENU_STRINGS = [
    ("CAT_NETWORK_RECON", "网络侦察"),
    ("CAT_RF_ANALYSIS", "射频分析"),
    ("CAT_WIFI_ATTACK", "WiFi攻击"),
    ("CAT_CRYPTO", "加密工具"),
    ("CAT_HW_DIAG", "硬件诊断"),
    ("CAT_NET_DIAG", "网络诊断"),
    ("CAT_433MHZ", "433MHz射频"),
    ("CAT_SYSTEM", "系统工具"),
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
    ("MOD_BEACON_DEC", "信标解码"),
    ("MOD_FRAME_CNT", "帧计数器"),
    ("MOD_RSSI_DIST", "距离估算"),
    ("MOD_CH_QUALITY", "信道质量"),
    ("MOD_RSSI_HIST", "信号直方图"),
    ("MOD_SPECTRUM", "频谱视图"),
    ("MOD_CH_HOPPER", "信道跳频"),
    ("MOD_SIG_MON", "信号监控"),
    ("MOD_RF_NOISE", "射频噪声"),
    ("MOD_WIFI_ATTACK", "WiFi攻击中心"),
    ("WA_SCAN_TARGETS", "扫描目标"),
    ("WA_SELECT_TARGET", "选择目标"),
    ("WA_ATTACK_MODE", "攻击模式"),
    ("WA_CONFIGURE", "配置参数"),
    ("WA_LAUNCH", "开始攻击"),
    ("WA_DEV_TESTS", "设备测试"),
    ("ATK_DEAUTH", "解除认证"),
    ("ATK_BEACON", "信标洪水"),
    ("ATK_PMKID", "PMKID捕获"),
    ("ATK_HANDSHAKE", "握手捕获"),
    ("ATK_ROGUE", "伪基站"),
    ("ATK_KARMA", "Karma攻击"),
    ("ATK_EVIL_TWIN", "钓鱼热点"),
    ("MOD_TOTP", "动态口令"),
    ("MOD_PASSWD", "密码生成"),
    ("MOD_MAC_OUI", "厂商查询"),
    ("MOD_BASE64", "Base64编码"),
    ("MOD_HEX", "十六进制"),
    ("MOD_HASH", "哈希计算"),
    ("MOD_XOR", "异或加密"),
    ("MOD_LOGIC_PROBE", "逻辑探针"),
    ("MOD_PWM", "PWM发生器"),
    ("MOD_ADC", "ADC电压"),
    ("MOD_I2C", "I2C扫描"),
    ("MOD_GPIO", "GPIO控制"),
    ("MOD_UART", "串口监视"),
    ("MOD_SPI", "SPI扫描"),
    ("MOD_FREQ", "频率发生器"),
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
    ("MOD_433MHZ", "433MHz工具箱"),
    ("RF_SCAN_433", "扫描433"),
    ("RF_BROWSE", "已捕获码"),
    ("RF_SEND", "发送433"),
    ("RF_RAW_EDIT", "原始编辑"),
    ("RF_DEV_DB", "设备库"),
    ("RF_TEMP_SENSOR", "温度传感"),
    ("MOD_SYS_INFO", "系统信息"),
    ("MOD_FILE_MGR", "文件管理"),
    ("MOD_SCR_TEST", "屏幕测试"),
    ("MOD_BTN_TEST", "按键测试"),
    ("MOD_BATTERY", "电池电压"),
    ("MOD_SLEEP", "休眠定时"),
    ("MOD_AUTO_CFG", "自动配置"),
    ("MOD_LED_TEST", "LED测试"),
    ("MOD_SETTINGS", "系统设置"),
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
]


def is_cjk(c):
    """Check if character is CJK (needs Chinese font rendering)."""
    cp = ord(c)
    return (0x4E00 <= cp <= 0x9FFF or 0x3400 <= cp <= 0x4DBF or
            0x20000 <= cp <= 0x2A6DF or 0xF900 <= cp <= 0xFAFF or
            0x2F800 <= cp <= 0x2FA1F or 0x3000 <= cp <= 0x303F or
            0xFF00 <= cp <= 0xFFEF or 0x2E80 <= cp <= 0x2EFF or
            0x31C0 <= cp <= 0x31EF)


def render_glyph(char, font):
    """Render a single character as an XBM bitmap. Returns (w, h, hex_lines)."""
    bbox = font.getbbox(char)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]

    if w < 1: w = 1
    if h < 1: h = 1

    img = Image.new('1', (w, h), 0)
    draw = ImageDraw.Draw(img)
    draw.text((-bbox[0], -bbox[1]), char, font=font, fill=1)

    lines = []
    row_bytes = (w + 7) // 8
    for y in range(h):
        for bx in range(row_bytes):
            val = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < w and img.getpixel((x, y)):
                    val |= (1 << bit)
            lines.append(f"0x{val:02X}")

    return w, h, lines


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    font = ImageFont.truetype(FONT_PATH, FONT_SIZE)

    # Collect all unique Chinese characters
    all_chars = set()
    for _, text in MENU_STRINGS:
        for c in text:
            if is_cjk(c):
                all_chars.add(c)

    # Also add ASCII chars that appear in Chinese strings
    ascii_in_cn = set()
    for _, text in MENU_STRINGS:
        for c in text:
            if not is_cjk(c) and c != ' ':
                ascii_in_cn.add(c)

    char_list = sorted(all_chars)
    print(f"Unique CJK chars: {len(char_list)}")
    print(f"ASCII chars in strings: {sorted(ascii_in_cn)}")
    print(f"Chars: {''.join(char_list)}")

    # Render each glyph
    glyphs = {}  # char -> (cp, w, h, data)
    for ch in char_list:
        w, h, data = render_glyph(ch, font)
        glyphs[ch] = (ord(ch), w, h, data)

    # Compute font metrics
    max_w = max(g[1] for g in glyphs.values())
    max_h = max(g[2] for g in glyphs.values())
    total_data_bytes = sum(len(g[3]) for g in glyphs.values())
    print(f"Max glyph: {max_w}x{max_h}, total data: {total_data_bytes} bytes")

    # Write header file
    with open(f"{OUTPUT_DIR}/chinese_glyphs.h", 'w', encoding='utf-8') as f:
        f.write('''#pragma once
#include <Arduino.h>
#include <U8g2lib.h>

// Per-character Chinese glyph bitmaps for ESP8266 OLED
// Generated by tools/gen_cn_glyphs.py
// Each glyph is a small XBM drawn with u8g2.drawXBM()

struct CNGlyph {
    const uint8_t* data;    // offset 0 (4B, aligned) — must be first
    uint16_t unicode;       // offset 4 (2B, aligned)
    uint8_t w, h;           // offset 6,7 (1B each)
};

// Draw a UTF-8 Chinese string character by character
void drawCN(U8G2& u8g2, uint8_t x, uint8_t y, const char* utf8);
uint8_t cnStrWidth(const char* utf8);

#define CN_GLYPH_COUNT %d
#define CN_GLYPH_MAX_W %d
#define CN_GLYPH_MAX_H %d

extern const CNGlyph cn_glyph_table[] PROGMEM;

''' % (len(glyphs), max_w, max_h))

        # Declare glyph data arrays
        for ch in char_list:
            cp = ord(ch)
            f.write(f'extern const uint8_t cn_g{cp:04X}[] PROGMEM;\n')

    # Write CPP file
    with open(f"{OUTPUT_DIR}/chinese_glyphs.cpp", 'w', encoding='utf-8') as f:
        f.write('#include "chinese_glyphs.h"\n#include "config.h"\n\n')

        # Write glyph data
        for ch in char_list:
            cp = ord(ch)
            w, h, data_lines = glyphs[ch][1], glyphs[ch][2], glyphs[ch][3]
            f.write(f'// U+{cp:04X} {ch} ({w}x{h})\n')
            f.write(f'const uint8_t cn_g{cp:04X}[] PROGMEM = {{\n')
            for i in range(0, len(data_lines), 16):
                chunk = data_lines[i:i+16]
                f.write('  ' + ','.join(chunk) + ',\n')
            f.write('};\n\n')

        # Write lookup table
        f.write('const CNGlyph cn_glyph_table[] PROGMEM = {\n')
        for ch in char_list:
            cp = ord(ch)
            w, h = glyphs[ch][1], glyphs[ch][2]
            f.write(f'  {{cn_g{cp:04X}, {cp:#06X}, {w}, {h}}},\n')
        f.write('};\n\n')

        # Write draw function
        f.write('''
// Decode UTF-8 to Unicode code point, return bytes consumed
static uint16_t utf8_decode(const char* s, uint16_t* cp) {
    uint8_t c = (uint8_t)*s;
    if (c < 0x80) { *cp = c; return 1; }
    if ((c & 0xE0) == 0xC0) {
        *cp = ((uint16_t)(c & 0x1F) << 6) | (s[1] & 0x3F);
        return 2;
    }
    if ((c & 0xF0) == 0xE0) {
        *cp = ((uint16_t)(c & 0x0F) << 12) | ((uint16_t)(s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        return 3;
    }
    *cp = 0xFFFD;
    return 1;
}

static const CNGlyph* cn_find_glyph(uint16_t unicode) {
    for (uint8_t i = 0; i < CN_GLYPH_COUNT; i++) {
        const CNGlyph* g = &cn_glyph_table[i];
        uint16_t cu = pgm_read_word(&g->unicode);
        if (cu == unicode) return g;
    }
    return nullptr;
}

void drawCN(U8G2& u8g2, uint8_t x, uint8_t y, const char* utf8) {
    uint8_t cx = x;
    while (*utf8) {
        uint16_t cp;
        uint8_t adv = utf8_decode(utf8, &cp);
        utf8 += adv;

        if (cp < 0x80) {
            // ASCII char - use FONT_MENU for proper sizing
            u8g2.setFont(FONT_MENU);
            char ascii[2] = {(char)cp, '\\0'};
            u8g2.drawStr(cx, y - 2, ascii);
            cx += u8g2.getStrWidth(ascii) + 1;
            continue;
        }

        const CNGlyph* g = cn_find_glyph(cp);
        if (!g) continue;

        // Copy struct + bitmap data from PROGMEM to stack
        CNGlyph local;
        memcpy_P(&local, g, sizeof(CNGlyph));
        uint8_t buf[32];
        uint8_t buf_len = ((local.w + 7) / 8) * local.h;
        memcpy_P(buf, local.data, buf_len);
        u8g2.drawXBM(cx, y - local.h, local.w, local.h, buf);
        cx += local.w + 1;
    }
}

uint8_t cnStrWidth(const char* utf8) {
    uint8_t w = 0;
    while (*utf8) {
        uint16_t cp;
        uint8_t adv = utf8_decode(utf8, &cp);
        utf8 += adv;
        if (cp < 0x80) {
            w += 10;  // approximate FONT_MENU char width + gap
            continue;
        }
        const CNGlyph* g = cn_find_glyph(cp);
        if (g) w += pgm_read_byte(&g->w) + 1;
    }
    if (w > 0) w--;  // remove trailing gap
    return w;
}
''')

    print(f"\nGenerated {len(glyphs)} glyphs")
    print(f"Output: {OUTPUT_DIR}/chinese_glyphs.h, {OUTPUT_DIR}/chinese_glyphs.cpp")


if __name__ == '__main__':
    main()
