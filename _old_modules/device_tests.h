#pragma once
#include "module_interface.h"
#include "../icons.h"

// Sub-test types
enum DeviceTestType : uint8_t {
    TEST_CAMERA_INTERFERENCE = 0,
    TEST_CONNECTION_ROBUSTNESS,
    TEST_LEAVE_DETECTION,
    TEST_ARP_SPOOF_DETECT,
    TEST_WPS_LOCK_CHECK,
    TEST_SYN_FLOOD,
    TEST_UDP_FLOOD,
    TEST_DEFAULT_CREDS,
    TEST_IOT_PORT_SCAN,
    TEST_COUNT
};

class DeviceTests : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 2; }
    const char* getName() const override     { return "Device Tests"; }
    const char* getTitle() const override    { return "Device Tests"; }
    const unsigned char* getIcon() const override { return icon_tests; }
    uint8_t     getId() const override       { return 47; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint8_t  selectedTest;
    bool     running;
    bool     confirmed;
    uint32_t startTime;
    char     statusMsg[48];
    uint8_t  testData;  // test context data

    const char* getTestName(uint8_t idx);
    void startSelectedTest();
    void stopTest();
    void updateCameraTest();
    void updateConnectionTest();
    void updateLeaveDetection();
    void updateArpDetect();
    void updateWpsCheck();
    void updateSynFlood();
    void updateUdpFlood();
    void updateDefaultCreds();
    void updateIotScan();
};

ModuleInterface* createDeviceTests();
