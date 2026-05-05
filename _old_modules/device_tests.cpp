#include "device_tests.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../utils.h"

ModuleInterface* createDeviceTests() { return new DeviceTests(); }

void DeviceTests::init() {
    selectedTest = 0; running = false; confirmed = false;
    startTime = 0; statusMsg[0] = '\0'; testData = 0;
}

void DeviceTests::enter() {
    selectedTest = 0; running = false; confirmed = false;
    statusMsg[0] = '\0'; testData = 0;
    displayMgr.setDirty();
}

void DeviceTests::exit() {
    if (running) stopTest();
}

const char* DeviceTests::getTestName(uint8_t idx) {
    switch (idx) {
        case TEST_CAMERA_INTERFERENCE:  return "Wi-Fi Camera Interference";
        case TEST_CONNECTION_ROBUSTNESS: return "Connection Robustness";
        case TEST_LEAVE_DETECTION:      return "Leave Detection";
        case TEST_ARP_SPOOF_DETECT:     return "ARP Spoof Detection";
        case TEST_WPS_LOCK_CHECK:       return "WPS Lock Detection";
        case TEST_SYN_FLOOD:            return "SYN Flood Demo";
        case TEST_UDP_FLOOD:            return "UDP Flood Demo";
        case TEST_DEFAULT_CREDS:        return "Default Credential Check";
        case TEST_IOT_PORT_SCAN:        return "IoT Port Scan";
        default: return "Unknown Test";
    }
}

void DeviceTests::startSelectedTest() {
    if (running) return;
    running = true;
    confirmed = false;
    startTime = millis();
    testData = 0;
    strCopySafe(statusMsg, "Test running...", sizeof(statusMsg));
    displayMgr.setDirty();
}

void DeviceTests::stopTest() {
    running = false;
    strCopySafe(statusMsg, "Test stopped", sizeof(statusMsg));
    wifiMgr.forceRelease();
    displayMgr.setDirty();
}

void DeviceTests::updateCameraTest() {
    // Send few deauth to camera, observe recovery
    // Simplified impl
}

void DeviceTests::updateConnectionTest() {
    // Repeated deauth phone, log reconnect time
}

void DeviceTests::updateLeaveDetection() {
    // Listen for Probe Req from specific MAC
}

void DeviceTests::updateArpDetect() {
    // Listen ARP, alert on GW MAC change
    if (wifiMgr.getState() == WM_PROMISCUOUS) {
        // Check ARP in promisc callback
    }
}

void DeviceTests::updateWpsCheck() {
    // Send WPS probe
}

void DeviceTests::updateSynFlood() {
    // TCP SYN flood to self-hosted server
}

void DeviceTests::updateUdpFlood() {
    // High-speed UDP flood
}

void DeviceTests::updateDefaultCreds() {
    // Dictionary brute HTTP/Telnet creds
}

void DeviceTests::updateIotScan() {
    // IoT port scan: 21,22,23,80,443,502,1883
}

void DeviceTests::update() {
    if (!running) return;

    if (millis() - startTime > SEC_ATTACK_MAX_DURATION) {
        stopTest();
        return;
    }
}

void DeviceTests::handleButton(ButtonEvent ev) {
    if (running) {
        if (ev != BTN_NONE) stopTest();
        return;
    }
    switch (ev) {
        case BTN_UP_SHORT:
            if (selectedTest > 0) selectedTest--;
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (selectedTest < TEST_COUNT - 1) selectedTest++;
            displayMgr.setDirty();
            break;
        case BTN_OK_SHORT:
            if (!confirmed) { confirmed = true; displayMgr.setDirty(); }
            else { startSelectedTest(); }
            break;
        default: break;
    }
}

void DeviceTests::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Device Tests");
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 24, "⚠ Test Mode - Own Devices Only ⚠");

    if (running) {
        uint32_t remaining = (SEC_ATTACK_MAX_DURATION - (millis() - startTime)) / 1000;
        char buf[48];
        snprintf(buf, sizeof(buf), "%s", getTestName(selectedTest));
        u8g2.drawStr(2, 39, buf);
        snprintf(buf, sizeof(buf), "Left:%lus any key=stop", remaining);
        u8g2.drawStr(2, 63, buf);
    } else {
        u8g2.setFont(FONT_BODY);
        const char* name = getTestName(selectedTest);
        u8g2.drawStr(2, 39, name);

        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(2, 63, confirmed ? "Press OK start  UP/DN sel" : "OK confirm  UP/DN select");
    }
}
