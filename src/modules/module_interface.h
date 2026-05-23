#pragma once
#include <U8g2lib.h>
#include <stdint.h>

// Button event types
enum ButtonEvent : uint8_t {
    BTN_NONE            = 0,
    BTN_UP_SHORT        = 0x01,
    BTN_DOWN_SHORT      = 0x02,
    BTN_OK_SHORT        = 0x04,
    BTN_UP_LONG         = 0x11,
    BTN_DOWN_LONG       = 0x12,
    BTN_OK_LONG         = 0x14,
    BTN_UP_DOUBLE       = 0x21,
    BTN_DOWN_DOUBLE     = 0x22,
    BTN_OK_DOUBLE       = 0x24,
    BTN_UP_REPEAT       = 0x31,
    BTN_DOWN_REPEAT     = 0x32,
};

// Refresh mode (tells DisplayMgr when to redraw)
enum RefreshMode : uint8_t {
    REFRESH_ON_DEMAND   = 0,  // redraw on data change
    REFRESH_CONTINUOUS  = 1,  // redraw every frame (animations)
};

// System state
enum SystemState : uint8_t {
    STATE_BOOT          = 0,
    STATE_MENU_MAIN     = 1,
    STATE_MENU_CATEGORY = 2,
    STATE_TOOL_RUNNING  = 3,
};

// ============================================================
// Module abstract interface — all tools must implement
// ============================================================
class ModuleInterface {
public:
    virtual ~ModuleInterface() = default;

    // ---- Lifecycle ----
    virtual void init()    = 0;   // one-time init at boot
    virtual void enter()   = 0;   // called when user selects tool
    virtual void exit()    = 0;   // called when leaving (release WiFi/files)
    virtual void update()  = 0;   // call each loop, must be non-blocking
    virtual void draw(U8G2& u8g2) = 0;  // draw between firstPage/nextPage

    // ---- Input ----
    virtual void handleButton(ButtonEvent ev) = 0;

    // Return true if module handled back internally (e.g. sub-state → main menu).
    // Default: return false → system exits the module.
    virtual bool handleBack() { return false; }

    // ---- Metadata ----
    virtual uint8_t     getCategory()    const = 0;  // 0-7
    virtual const char* getName()        const = 0;  // display name, <=20 chars
    virtual uint8_t     getId()          const = 0;  // unique ID (0-88)
    virtual const char* getTitle()       const = 0;  // runtime title
    virtual RefreshMode getRefreshMode() const = 0;

    // ---- Icon (XBM 16x16, PROGMEM) ----
    virtual const unsigned char* getIcon()       const { return nullptr; }
    virtual uint8_t              getIconWidth()  const { return 16; }
    virtual uint8_t              getIconHeight() const { return 16; }

    // Runtime options
    virtual bool wantsStatusBar()  const { return true; }
    virtual bool canRunOffline()   const { return false; }  // works offline?
};

// Factory function type
typedef ModuleInterface* (*ModuleFactory)();
