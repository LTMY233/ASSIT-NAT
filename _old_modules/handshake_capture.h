#pragma once
#include "module_interface.h"
#include "../icons.h"

class HandshakeCapture : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 2; }
    const char* getName() const override     { return "Handshake Capture"; }
    const char* getTitle() const override    { return "WPA Handshake Sniff"; }
    const unsigned char* getIcon() const override { return icon_handshake; }
    uint8_t     getId() const override       { return 43; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    bool     running;
    bool     captured;
    uint8_t  handshake[4];  // 4 EAPOL frame capture states (bitmask)
    uint32_t captureStart;
    uint8_t  targetBssid[6];
    uint8_t  pcapBuf[512];  // simple pcap buffer
    uint16_t pcapLen;

    static void onPacket(uint8_t* buf, uint16_t len);
    static HandshakeCapture* instance;
    void processEapol(uint8_t* buf, uint16_t len);
    void writePcapHeader();
    void appendPcapFrame(uint8_t* buf, uint16_t len);
};

ModuleInterface* createHandshakeCapture();
