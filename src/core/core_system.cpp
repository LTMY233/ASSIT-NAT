#include "core_system.h"
#include "config.h"
#include "button_mgr.h"
#include "display_mgr.h"
#include "menu_engine.h"
#include "sw_rtc.h"
#include "wifi_mgr.h"
#include "event_bus.h"
#include "modules/module_registry.h"
#include "utils.h"

#include "../modules/wifi_scanner.h"
#include "../modules/channel_heatmap.h"
#include "../modules/probe_sniffer.h"
#include "../modules/host_discovery.h"
#include "../modules/gateway_monitor.h"
#include "../modules/port_scanner.h"
#include "../modules/traceroute.h"
#include "../modules/net_stats.h"
#include "../modules/dhcp_discovery.h"
#include "../modules/mdns_enum.h"
#include "../modules/upnp_discovery.h"
#include "../modules/beacon_decoder.h"
#include "../modules/frame_counter.h"
#include "../modules/rssi_distance.h"
#include "../modules/channel_quality.h"
#include "../modules/rssi_histogram.h"
#include "../modules/wifi_attack.h"
#include "../modules/totp.h"
#include "../modules/password_gen.h"
#include "../modules/mac_lookup.h"
#include "../modules/logic_probe.h"
#include "../modules/pwm_gen.h"
#include "../modules/adc_voltmeter.h"
#include "../modules/i2c_scanner.h"
#include "../modules/gateway_discovery.h"
#include "../modules/multi_gateway.h"
#include "../modules/channel_switch.h"
#include "../modules/rssi_drift.h"
#include "../modules/broadcast_storm.h"
#include "../modules/service_discovery.h"
#include "../modules/event_logger.h"
#include "../modules/net_snapshot.h"
#include "../modules/dns_lookup.h"
#include "../modules/http_client.h"
#include "../modules/ping_monitor.h"
#include "../modules/ntp_query.h"
#include "../modules/wake_on_lan.h"
#include "../modules/spectrum_view.h"
#include "../modules/channel_hopper.h"
#include "../modules/signal_monitor.h"
#include "../modules/rf_noise.h"
#include "../modules/base64.h"
#include "../modules/hex_convert.h"
#include "../modules/hash_calc.h"
#include "../modules/xor_encrypt.h"
#include "../modules/wps_pin.h"
#include "../modules/ssl_check.h"
#include "../modules/gpio_control.h"
#include "../modules/uart_monitor.h"
#include "../modules/spi_scanner.h"
#include "../modules/freq_generator.h"
#include "../modules/speed_test.h"
#include "../modules/subnet_calc.h"
#include "../modules/rf433_tool.h"
#include "../modules/system_info.h"
#include "../modules/file_manager.h"
#include "../modules/screen_test.h"
#include "../modules/button_test.h"
#include "../modules/battery_adc.h"
#include "../modules/sleep_timer.h"
#include "../modules/auto_config.h"
#include "../modules/led_test.h"
#include "../modules/settings.h"

CoreSystem coreSystem;

void CoreSystem::init() {
    Serial.begin(115200);
    Serial.println("\n[ESP8266] ASSIT-NAT");
    delay(500);

    lastState = STATE_BOOT;
    heapCheckTimer = millis();

    if (!lfsInit()) {
        Serial.println("[ERROR] LittleFS mount failed!");
    }

    displayMgr.init();
    displayMgr.update(STATE_BOOT, nullptr);

    buttonMgr.init(PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_OK);
    eventBus.init();
    wifiMgr.init();
    swRTC.init();
    menuEngine.init();
    moduleRegistry.init();

    moduleRegistry.registerModule(createWifiScanner());
    moduleRegistry.registerModule(createChannelHeatmap());
    moduleRegistry.registerModule(createProbeSniffer());
    moduleRegistry.registerModule(createHostDiscovery());
    moduleRegistry.registerModule(createGatewayMonitor());
    moduleRegistry.registerModule(createPortScanner());
    moduleRegistry.registerModule(createTraceroute());
    moduleRegistry.registerModule(createNetStats());
    moduleRegistry.registerModule(createDhcpDiscovery());
    moduleRegistry.registerModule(createMdnsEnum());
    moduleRegistry.registerModule(createUpnpDiscovery());
    moduleRegistry.registerModule(createDnsLookup());
    moduleRegistry.registerModule(createHttpClient());
    moduleRegistry.registerModule(createPingMonitor());
    moduleRegistry.registerModule(createNtpQuery());
    moduleRegistry.registerModule(createWakeOnLan());

    moduleRegistry.registerModule(createBeaconDecoder());
    moduleRegistry.registerModule(createFrameCounter());
    moduleRegistry.registerModule(createRssiDistance());
    moduleRegistry.registerModule(createChannelQuality());
    moduleRegistry.registerModule(createRssiHistogram());
    moduleRegistry.registerModule(createSpectrumView());
    moduleRegistry.registerModule(createChannelHopper());
    moduleRegistry.registerModule(createSignalMonitor());
    moduleRegistry.registerModule(createRfNoise());

    moduleRegistry.registerModule(createWifiAttack());
    moduleRegistry.registerModule(createWpsPinCalc());
    moduleRegistry.registerModule(createSslCheck());

    moduleRegistry.registerModule(createTotpGenerator());
    moduleRegistry.registerModule(createPasswordGen());
    moduleRegistry.registerModule(createMacLookup());
    moduleRegistry.registerModule(createBase64Tool());
    moduleRegistry.registerModule(createHexConvert());
    moduleRegistry.registerModule(createHashCalc());
    moduleRegistry.registerModule(createXorEncrypt());

    moduleRegistry.registerModule(createLogicProbe());
    moduleRegistry.registerModule(createPwmGen());
    moduleRegistry.registerModule(createAdcVoltmeter());
    moduleRegistry.registerModule(createI2cScanner());
    moduleRegistry.registerModule(createGpioControl());
    moduleRegistry.registerModule(createUartMonitor());
    moduleRegistry.registerModule(createSpiScanner());
    moduleRegistry.registerModule(createFreqGenerator());

    moduleRegistry.registerModule(createGatewayDiscovery());
    moduleRegistry.registerModule(createMultiGateway());
    moduleRegistry.registerModule(createChannelSwitch());
    moduleRegistry.registerModule(createRssiDrift());
    moduleRegistry.registerModule(createBroadcastStorm());
    moduleRegistry.registerModule(createServiceDiscovery());
    moduleRegistry.registerModule(createEventLogger());
    moduleRegistry.registerModule(createNetSnapshot());
    moduleRegistry.registerModule(createSpeedTest());
    moduleRegistry.registerModule(createSubnetCalc());

    moduleRegistry.registerModule(createRf433Tool());

    moduleRegistry.registerModule(createSystemInfo());
    moduleRegistry.registerModule(createFileManager());
    moduleRegistry.registerModule(createScreenTest());
    moduleRegistry.registerModule(createButtonTest());
    moduleRegistry.registerModule(createBatteryAdc());
    moduleRegistry.registerModule(createSleepTimer());
    moduleRegistry.registerModule(createAutoConfig());
    moduleRegistry.registerModule(createLedTest());
    moduleRegistry.registerModule(createSettings());

    Serial.printf("[ESP8266] %d modules registered.\n", moduleRegistry.getCount());

    wifiMgr.setStationMode();
    Serial.println("[ESP8266] Ready.");

    displayMgr.update(STATE_BOOT, nullptr);
    delay(2000);

    menuEngine.buildMenu();

    lastState = STATE_MENU_MAIN;
    displayMgr.setDirty();
}

void CoreSystem::processButtons() {
    buttonMgr.update();

    ButtonEvent ev;
    while (buttonMgr.getEvent(ev)) {
        if (ev == BTN_OK_LONG || ev == BTN_OK_DOUBLE) {
            if (moduleRegistry.active() && moduleRegistry.active()->handleBack()) {
                continue;
            }
            menuEngine.navigateBack();
            continue;
        }
        if (moduleRegistry.active()) {
            moduleRegistry.active()->handleButton(ev);
        } else {
            menuEngine.handleButton(ev);
        }
    }
}

void CoreSystem::checkHeap() {
    uint32_t now = millis();
    if (now - heapCheckTimer > 60000) {
        heapCheckTimer = now;
        uint32_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < HEAP_WARN_THRESHOLD) {
            Serial.printf("[WARN] Low heap: %d bytes\n", freeHeap);
        }
    }
}

void CoreSystem::tick() {
    processButtons();

    if (moduleRegistry.active()) {
        moduleRegistry.active()->update();
    } else {
        menuEngine.update();
    }

    SystemState currentState;
    if (moduleRegistry.active()) {
        currentState = STATE_TOOL_RUNNING;
    } else {
        currentState = menuEngine.getState();
    }

    swRTC.update();
    wifiMgr.update();

    displayMgr.update(currentState, moduleRegistry.active());

    checkHeap();
    yield();

    lastState = currentState;
}
