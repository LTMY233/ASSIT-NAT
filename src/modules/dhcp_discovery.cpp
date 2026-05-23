#include "../chinese_glyphs.h"
#include "dhcp_discovery.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createDhcpDiscovery() { return new DhcpDiscovery(); }

void DhcpDiscovery::init() {
    memset(servers, 0, sizeof(servers));
    serverCount = 0; scanning = false; scanStart = 0;
}

void DhcpDiscovery::enter() {
    serverCount = 0; scanning = false;
    sendDiscover();
    displayMgr.setDirty();
}

void DhcpDiscovery::exit() {
    udp.stop();
}

void DhcpDiscovery::sendDiscover() {
    // Build simplified DHCP Discover packet
    uint8_t packet[300] = {0};
    packet[0] = 1;       // BOOTREQUEST
    packet[1] = 1;       // Ethernet
    packet[2] = 6;       // MAC addr length
    packet[3] = 0;       // Hops

    // Transaction ID (random)
    *(uint32_t*)(packet + 4) = random(0xFFFFFFFF);

    // DHCP Magic Cookie
    packet[236] = 99; packet[237] = 130; packet[238] = 83; packet[239] = 99;

    // DHCP Discover (Option 53, len 1)
    packet[240] = 53; packet[241] = 1; packet[242] = 1;  // DHCPDISCOVER

    // Parameter Request List (Option 55)
    packet[243] = 55; packet[244] = 4;
    packet[245] = 1; packet[246] = 3; packet[247] = 6; packet[248] = 15;

    // End
    packet[249] = 255;

    udp.begin(68);
    udp.beginPacket(IPAddress(255, 255, 255, 255), 67);
    udp.write(packet, 250);
    udp.endPacket();

    scanStart = millis();
    scanning = true;
}

void DhcpDiscovery::parseOffer(uint8_t* buf, uint16_t len) {
    if (len < 250) return;
    if (buf[0] != 2) return;  // BOOTREPLY

    if (serverCount >= DHCP_MAX_SERVERS) return;

    // Parse options
    uint8_t* opts = buf + 240;
    uint16_t optEnd = len - 240;
    DhcpInfo& srv = servers[serverCount];

    srv.ip = IPAddress(buf[16], buf[17], buf[18], buf[19]);
    // Get yiaddr (assigned IP) as reference
    srv.offeredSubnet = IPAddress(255, 255, 255, 0);

    for (uint16_t i = 0; i < optEnd - 2; ) {
        uint8_t optCode = opts[i];
        if (optCode == 255) break;
        if (optCode == 0) { i++; continue; }
        uint8_t optLen = opts[i + 1];
        if (optCode == 1 && optLen == 4) {  // Subnet Mask
            srv.offeredSubnet = IPAddress(opts[i+2], opts[i+3], opts[i+4], opts[i+5]);
        } else if (optCode == 3 && optLen >= 4) {  // Router
            srv.offeredRouter = IPAddress(opts[i+2], opts[i+3], opts[i+4], opts[i+5]);
        } else if (optCode == 51 && optLen == 4) {  // Lease Time
            srv.leaseTime = ((uint32_t)opts[i+2] << 24) | ((uint32_t)opts[i+3] << 16) |
                           ((uint32_t)opts[i+4] << 8) | opts[i+5];
        }
        i += 2 + optLen;
    }

    serverCount++;
}

void DhcpDiscovery::update() {
    if (!scanning) return;

    // Check response
    if (udp.parsePacket()) {
        uint8_t buf[350];
        int len = udp.read(buf, 350);
        if (len > 240) parseOffer(buf, len);
        displayMgr.setDirty();
    }

    if (millis() - scanStart > 5000) {
        scanning = false;
        udp.stop();
        displayMgr.setDirty();
    }
}

void DhcpDiscovery::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT && !scanning) {
        serverCount = 0; sendDiscover(); displayMgr.setDirty();
    }
}

void DhcpDiscovery::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    if (scanning) {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 2, 30, "扫描中...");
        return;
    }

    if (serverCount == 0) {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 10, 35, "未发现DHCP");
    } else {
        u8g2.setFont(FONT_DATA);
        for (uint8_t i = 0; i < serverCount; i++) {
            uint8_t y = 24 + i * 15;
            char buf[40];
            snprintf(buf, sizeof(buf), "DHCP:%d.%d.%d.%d",
                     servers[i].ip[0], servers[i].ip[1],
                     servers[i].ip[2], servers[i].ip[3]);
            u8g2.drawStr(2, y, buf);
            snprintf(buf, sizeof(buf), "GW:%d.%d.%d.%d",
                     servers[i].offeredRouter[0], servers[i].offeredRouter[1],
                     servers[i].offeredRouter[2], servers[i].offeredRouter[3]);
            u8g2.drawStr(2, y + 15, buf);
        }
    }

    u8g2.setFont(FONT_DATA);
    drawCN(u8g2, 0, 63, "按OK重扫");
}
