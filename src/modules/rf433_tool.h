#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <RCSwitch.h>

#define RF433_MAX_CODES   16
#define RF433_MAX_PULSES  64
#define RF433_DB_COUNT    12

struct Rf433CodeEntry {
    unsigned long value;
    uint8_t  bitLength;
    uint8_t  protocol;
    uint32_t timestamp;
};

class Rf433Tool : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 6; }
    const char* getName()     const override { return "433MHz Tool"; }
    const char* getTitle()    const override;
    const unsigned char* getIcon() const override { return icon_deauth; }
    uint8_t     getId()       const override { return 70; }
    RefreshMode getRefreshMode() const override;

private:
    enum RfState : uint8_t {
        RF_MENU,
        RF_SCAN,
        RF_BROWSE,
        RF_SEND,
        RF_RAW_EDIT,
        RF_DEVDB,
        RF_TEMP_SENSOR,
    };
    RfState state;

    // Menu
    uint8_t  menuCursor;

    // RX/TX tracking
    bool     rxActive;
    bool     txActive;

    // Captured codes
    Rf433CodeEntry codes[RF433_MAX_CODES];
    uint8_t   codeCount;
    uint8_t   codeCursor;

    // Scan data (local buffer during RF_SCAN)
    unsigned long scanLastVal;
    uint8_t  scanLastBits;
    uint8_t  scanLastProto;
    uint32_t scanSignalCnt;
    int      scanRaw[RF433_MAX_PULSES];
    uint8_t  scanRawCnt;
    uint16_t scanAvgUs;

    // TX data
    unsigned long txCode;
    uint8_t  txBits;
    uint8_t  txProto;
    uint8_t  txPulseUs;
    uint8_t  txEditPos;
    uint8_t  txEditMode;  // 0=bit, 1=proto, 2=pulse
    uint32_t txCount;

    // Dev DB
    uint8_t  dbCursor;
    bool     dbDetail;

    // Menu animation
    float    mScrollY, mScrollYTarget;
    float    mSelY, mSelYTarget;
    float    mSelW, mSelWTarget;
    uint32_t mAnimLastMs;

    // Temp sensor
    float    tempVal;
    uint8_t  humidity;
    char     tempType[16];
    uint32_t tempTs;

    // RCSwitch
    RCSwitch rcSwitch;

    // Helpers
    void enableRX();
    void disableRX();
    void enableTX();
    void disableTX();
    void addCode(unsigned long val, uint8_t bits, uint8_t proto);
    void analyzePulses();
    const char* guessProtocol(uint16_t avgUs);
    const char* protoName(uint8_t proto);
    void bitsToStr(unsigned long val, uint8_t bits, char* buf, size_t sz);
    void tryDecodeTemp(unsigned long val, uint8_t bits, uint8_t proto);

    void recomputeMenuTargets();
    void drawMenu(U8G2& u8g2);
    void drawScan(U8G2& u8g2);
    void drawBrowse(U8G2& u8g2);
    void drawSend(U8G2& u8g2);
    void drawRawEdit(U8G2& u8g2);
    void drawDevDb(U8G2& u8g2);
    void drawTemp(U8G2& u8g2);
};

ModuleInterface* createRf433Tool();
