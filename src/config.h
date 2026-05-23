#pragma once
#include <Arduino.h>

// ============================================================
// Pin definitions (NodeMCU / Wemos D1 mini)
// ============================================================
#define PIN_BTN_UP      D3   // GPIO0  - UP (ext pull-down, internal pull-up)
#define PIN_BTN_DOWN    D4   // GPIO2  - DOWN
#define PIN_BTN_OK      D5   // GPIO14 - OK/BACK
#define PIN_OLED_SDA    D2   // GPIO4  - I2C SDA
#define PIN_OLED_SCL    D1   // GPIO5  - I2C SCL
#define PIN_PWM_OUT     D8   // GPIO15 - PWM output
#define PIN_GPIO_PROBE  A0   // ADC / logic probe
#define PIN_ADC_IN      A0   // ADC input (TOUT, 0-1V)
#define PIN_RX433       12   // GPIO12/D6 - 433MHz RX (SYN480R)
#define PIN_TX433       13   // GPIO13/D7 - 433MHz TX (F115)

// ============================================================
// Display
// ============================================================
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_I2C_ADDR   0x3C

// ============================================================
// Menu layout
// ============================================================
#define MENU_VISIBLE        3     // 3 rows visible
#define MENU_ROW_HEIGHT     18    // row height (px)
#define MENU_TOP_PAD         0    // no title bar
#define MENU_TITLE_H         9    // title text area
#define MENU_ICON_X          4    // icon X position
#define MENU_TEXT_X         24    // text X (icon + gap)
#define MENU_SCROLLBAR_X   125    // scrollbar X
#define MENU_SCROLLBAR_W     3    // scrollbar width
#define MENU_SEL_RADIUS      3    // selection box radius
#define MENU_SEL_PAD_X       5    // selection box h-pad
#define MENU_ICON_W         16
#define MENU_ICON_H         16

// ============================================================
// Button debounce
// ============================================================
#define BTN_DEBOUNCE_MS     50
#define BTN_LONG_PRESS_MS   600
#define BTN_DOUBLE_GAP_MS   400
#define BTN_REPEAT_DELAY_MS 500
#define BTN_REPEAT_RATE_MS  100

// ============================================================
// Animation (time-driven exponential decay)
// ============================================================
#define ANIM_SPEED          8.0f   // decay rate (higher = faster)
#define ANIM_DT_MAX         0.05f  // deltaTime cap (sec), anti-glitch
#define ANIM_SNAP_THRESH    0.2f   // pixel snap threshold

// ============================================================
// I2C speed
// ============================================================
#define I2C_SPEED_HZ        800000 // HW I2C 800kHz

// ============================================================
// WiFi / Network
// ============================================================
#define WIFI_SCAN_TIMEOUT   4000
#define WIFI_SCAN_MAX_AP    50
#define WIFI_CHANNEL_MIN    1
#define WIFI_CHANNEL_MAX    13

// ============================================================
// Software RTC
// ============================================================
#define NTP_SERVER          "pool.ntp.org"
#define NTP_PORT            123
#define NTP_TIMEOUT         3000
#define NTP_RESYNC_INTERVAL 3600
#define NTP_RETRY_INTERVAL  60000
#define RTC_CALIBRATION_FILE "/rtc.dat"
#define UTC_OFFSET_SEC      28800

// ============================================================
// Security tool limits
// ============================================================
#define SEC_ATTACK_MAX_DURATION  30000
#define SEC_DEAUTH_MAX_COUNT     50
#define SEC_BEACON_FLOOD_DUR     60000

// ============================================================
// LittleFS
// ============================================================
#define LFS_MOUNT_POINT     "/"
#define LFS_FORMAT_ON_FAIL  true
#define LFS_MAX_OPEN_FILES  5

// ============================================================
// System limits
// ============================================================
#define MENU_MAX_CATEGORIES    8
#define MENU_MAX_ITEMS_PER     20
#define MODULE_MAX_COUNT       80
#define EVENT_QUEUE_SIZE       16
#define BUTTON_EVENT_QUEUE     8
#define LOG_MAX_ENTRIES        256
#define HEAP_WARN_THRESHOLD    4096

// ============================================================
// Fonts
// ============================================================
#include <U8g2lib.h>
#define FONT_TITLE    u8g2_font_6x10_tf
#define FONT_BODY     u8g2_font_5x7_tf
#define FONT_MONO     u8g2_font_5x8_tf
#define FONT_SMALL    u8g2_font_4x6_tf
#define FONT_BIG      u8g2_font_10x20_tf
#define FONT_STATUS   u8g2_font_5x7_tf
#define FONT_MENU     u8g2_font_9x15B_tf   // menu bold large
#define FONT_DATA     u8g2_font_7x13B_tf   // data bold