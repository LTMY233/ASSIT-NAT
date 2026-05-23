#include "../chinese_glyphs.h"
#include "subnet_calc.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createSubnetCalc() { return new SubnetCalc(); }

void SubnetCalc::init() {
    ip[0] = 192; ip[1] = 168; ip[2] = 1; ip[3] = 1;
    mask[0] = 255; mask[1] = 255; mask[2] = 255; mask[3] = 0;
    memset(network, 0, sizeof(network));
    memset(broadcast, 0, sizeof(broadcast));
    memset(firstHost, 0, sizeof(firstHost));
    memset(lastHost, 0, sizeof(lastHost));
    editOctet = 0;
    editingMask = false;
    cidr = 24;
    recalc();
}

void SubnetCalc::enter() {
    editOctet = 0;
    editingMask = false;
    recalc();
    displayMgr.setDirty();
}

void SubnetCalc::exit() {}

uint8_t SubnetCalc::maskToCidr() {
    uint8_t bits = 0;
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t m = mask[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (m & 0x80) bits++;
            m <<= 1;
        }
    }
    return bits;
}

void SubnetCalc::cidrToMask(uint8_t c) {
    if (c > 32) c = 32;
    cidr = c;
    memset(mask, 0, 4);
    for (uint8_t i = 0; i < c; i++) {
        uint8_t byteIdx = i / 8;
        uint8_t bitIdx  = i % 8;
        mask[byteIdx] |= (0x80 >> bitIdx);
    }
}

void SubnetCalc::recalc() {
    cidr = maskToCidr();

    for (uint8_t i = 0; i < 4; i++) {
        network[i]   = ip[i] & mask[i];
        broadcast[i] = ip[i] | (~mask[i]);
    }

    // First host = network + 1
    memcpy(firstHost, network, 4);
    firstHost[3]++;
    if (firstHost[3] == 0) { firstHost[2]++; if (firstHost[2] == 0) { firstHost[1]++; if (firstHost[1] == 0) firstHost[0]++; } }

    // Last host = broadcast - 1
    memcpy(lastHost, broadcast, 4);
    if (lastHost[3] == 0) { lastHost[2]--; lastHost[3] = 255; }
    else lastHost[3]--;
    if (lastHost[3] >= 255) { lastHost[2]--; lastHost[3] = 255; }
}

void SubnetCalc::update() {}

void SubnetCalc::handleButton(ButtonEvent ev) {
    uint8_t* target = editingMask ? mask : ip;

    switch (ev) {
        case BTN_UP_SHORT:
            if (target[editOctet] < 255) target[editOctet]++;
            recalc();
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (target[editOctet] > 0) target[editOctet]--;
            recalc();
            displayMgr.setDirty();
            break;
        case BTN_OK_SHORT:
            editOctet = (editOctet + 1) % 4;
            displayMgr.setDirty();
            break;
        case BTN_OK_DOUBLE:
            editingMask = !editingMask;
            displayMgr.setDirty();
            break;
        case BTN_UP_LONG:
            cidrToMask(cidr + 1);
            recalc();
            displayMgr.setDirty();
            break;
        case BTN_DOWN_LONG:
            cidrToMask(cidr > 0 ? cidr - 1 : 0);
            recalc();
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void SubnetCalc::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    u8g2.setFont(FONT_BODY);

    // IP
    char buf[32];
    uint8_t* edTarget = editingMask ? mask : ip;
    snprintf(buf, sizeof(buf), "%s %d.%d.%d.%d/%d",
             editingMask ? "Mask:" : "IP:",
             ip[0], ip[1], ip[2], ip[3], cidr);
    u8g2.drawStr(0, 24, buf);

    // Mask
    snprintf(buf, sizeof(buf), "Mask:%d.%d.%d.%d",
             mask[0], mask[1], mask[2], mask[3]);
    u8g2.drawStr(0, 34, buf);

    // Network
    snprintf(buf, sizeof(buf), "Net:%d.%d.%d.%d",
             network[0], network[1], network[2], network[3]);
    u8g2.drawStr(0, 42, buf);

    // Broadcast
    snprintf(buf, sizeof(buf), "Bc:%d.%d.%d.%d",
             broadcast[0], broadcast[1], broadcast[2], broadcast[3]);
    u8g2.drawStr(0, 50, buf);

    // Footer
    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "Edit:oct%d %s",
             editOctet, editingMask ? "MASK" : "IP");
    u8g2.drawStr(0, 63, buf);
}
