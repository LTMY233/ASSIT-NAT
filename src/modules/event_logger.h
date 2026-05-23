#pragma once
#include "module_interface.h"
#include "../icons.h"

#define EVT_LOG_MAX 20

struct LogEntry {
    uint32_t timestamp;
    char     message[48];
};

class EventLogger : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 5; }
    const char* getName() const override     { return "WiFi日志"; }
    const char* getTitle() const override    { return "WiFi日志"; }
    const unsigned char* getIcon() const override { return icon_log; }
    uint8_t     getId() const override       { return 66; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    LogEntry entries[EVT_LOG_MAX];
    uint8_t  entryCount;
    uint8_t  cursor;
    bool     running;

    static void onPacket(uint8_t* buf, uint16_t len);
    static EventLogger* instance;
    void addEntry(const char* msg);
};

ModuleInterface* createEventLogger();
