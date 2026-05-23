#include "../chinese_glyphs.h"
#include "mac_lookup.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../oui_data.h"
#include "../utils.h"

ModuleInterface* createMacLookup() { return new MacLookup(); }

void MacLookup::init() {
    memset(mac, 0, sizeof(mac));
    mac[0] = 0x04; mac[1] = 0xCF; mac[2] = 0x8C;  // default Espressif
    editPos = 5;
    vendorName[0] = '\0';
    found = false;
}

void MacLookup::enter() {
    editPos = 5;
    lookup();
    displayMgr.setDirty();
}

void MacLookup::exit() {}

void MacLookup::lookup() {
    found = false;
    vendorName[0] = '\0';

    for (size_t i = 0; i < OUI_TABLE_SIZE; i++) {
        OuiEntry entry;
        memcpy_P(&entry, &ouiTable[i], sizeof(OuiEntry));

        if (entry.prefix[0] == mac[0] && entry.prefix[1] == mac[1] && entry.prefix[2] == mac[2]) {
            strcpy_P(vendorName, ouiNameTable + entry.nameOffset);
            found = true;
            return;
        }
    }

    if (!found) {
        vendorName[0] = '\0';
    }
}

void MacLookup::update() {}

void MacLookup::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            editPos = (editPos + 1) % 6;  // switch edit pos
            displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            mac[editPos]++;
            lookup(); displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            mac[editPos]--;
            lookup(); displayMgr.setDirty();
            break;
        case BTN_OK_LONG:
            // reset
            mac[0] = 0x04; mac[1] = 0xCF; mac[2] = 0x8C;
            mac[3] = 0x00; mac[4] = 0x00; mac[5] = 0x00;
            editPos = 5;
            lookup(); displayMgr.setDirty();
            break;
        default: break;
    }
}

void MacLookup::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // MAC address display (editable)
    u8g2.setFont(FONT_BIG);
    char macBuf[18];
    snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X",
             mac[0], mac[1], mac[2]);
    uint8_t tw = u8g2.getStrWidth(macBuf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 28, macBuf);

    // remaining bytes
    u8g2.setFont(FONT_DATA);
    char restBuf[10];
    snprintf(restBuf, sizeof(restBuf), ":%02X:%02X:%02X", mac[3], mac[4], mac[5]);
    tw = u8g2.getStrWidth(restBuf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 36, restBuf);

    // vendor name
    if (!found) {
        drawCN(u8g2, 2, 46, "未知厂商");
    } else {
        u8g2.setFont(FONT_BODY);
        uint8_t vLen = strlen(vendorName);
        if (vLen > 20) {
            // truncate display
            char trunc[21];
            strncpy(trunc, vendorName, 20); trunc[20] = '\0';
            u8g2.drawStr(2, 46, trunc);
        } else {
            u8g2.drawStr(2, 46, vendorName);
        }
    }

}
