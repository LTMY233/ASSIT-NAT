#pragma once
#include "module_interface.h"
#include "wifi_scanner.h"
#include "../icons.h"

#define WF_ATK_MAX_TARGETS   30
#define WF_ATK_SSID_LIST_LEN 14

enum WifiAttackType : uint8_t {
    WFA_DEAUTH       = 0,
    WFA_BEACON_FLOOD = 1,
    WFA_PMKID        = 2,
    WFA_HANDSHAKE    = 3,
    WFA_ROGUE_AP     = 4,
    WFA_KARMA        = 5,
    WFA_EVIL_TWIN    = 6,
    WFA_DEV_TESTS    = 7,
    WFA_COUNT        = 8
};

class WifiAttack : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;
    bool handleBack() override;

    uint8_t     getCategory() const override { return 2; }
    const char* getName()     const override { return "WiFi攻击中心"; }
    const char* getTitle()    const override;
    const unsigned char* getIcon() const override { return icon_deauth; }
    uint8_t     getId()       const override { return 40; }
    RefreshMode getRefreshMode() const override;

private:
    enum SubState : uint8_t {
        SUB_MENU,
        SUB_SCANNING,
        SUB_SELECT_TARGET,
        SUB_SELECT_MODE,
        SUB_CONFIGURE,
        SUB_CONFIRM,
        SUB_ATTACKING,
        SUB_DONE,
    };
    SubState substate;

    // Scan results
    ScanRecord targets[WF_ATK_MAX_TARGETS];
    uint8_t    targetCount;
    uint8_t    targetCursor;
    uint8_t    targetScroll;

    // Multi-target selection
    uint32_t selectedMask;       // bitmask, bit N=1 means target N selected
    uint8_t  selectedCount;
    // Single selected target (for backward compat in mode select, etc.)
    uint8_t selBssid[6];
    char    selSsid[33];
    uint8_t selChannel;
    int8_t  selRssi;

    // Attack selection
    WifiAttackType atkType;
    uint8_t        menuCursor;

    // Config params
    uint8_t  deauthCount;
    uint8_t  beaconDurationSec;
    uint8_t  devTestIndex;
    uint8_t  configStep;

    // Runtime
    bool     attacking;
    bool     safetyConfirmed;
    bool     promiscHeld;
    uint32_t scanStartMs;
    uint32_t attackStart;
    uint32_t lastActionMs;
    uint32_t sentCount;
    uint32_t clientCount;
    uint32_t remainingSec;
    uint32_t lastPpsTime;
    uint32_t lastPpsCount;
    uint16_t currentPps;
    uint8_t  currentTargetIdx;  // for cycling through selected targets
    char     statusMsg[48];

    // Menu animation
    float    mScrollY, mScrollYTarget;
    float    mSelY, mSelYTarget;
    float    mSelW, mSelWTarget;
    uint32_t mAnimLastMs;

    // Per-attack context
    uint8_t  eapolState;        // handshake 4-way state tracker

    void startWifiScan();
    void processScanResult();
    void launchAttack();
    void stopAttack();
    void recomputeMenuTargets();

    void updateDeauth();
    void updateBeaconFlood();
    void updatePmkid();
    void updateHandshake();
    void updateRogueAp();
    void updateKarma();
    void updateEvilTwin();
    void updateDevTests();

    void drawMenu(U8G2& u8g2);
    void drawScanning(U8G2& u8g2);
    void drawSelectTarget(U8G2& u8g2);
    void drawSelectMode(U8G2& u8g2);
    void drawConfigure(U8G2& u8g2);
    void drawConfirm(U8G2& u8g2);
    void drawAttacking(U8G2& u8g2);
    void drawDone(U8G2& u8g2);

    static const char* atkTypeName(WifiAttackType t);
    static const char* atkTypeLabel(WifiAttackType t);
    static const char* fakeSsidList[WF_ATK_SSID_LIST_LEN];
};

ModuleInterface* createWifiAttack();
