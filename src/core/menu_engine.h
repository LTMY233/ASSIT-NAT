#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"
#include "modules/module_interface.h"

#define MENU_MAX_PAGES      9
#define MENU_MAX_ITEMS      12

struct MenuItem {
    const char*           label;
    uint8_t               toolId;
    const unsigned char*  icon;
};

struct MenuPage {
    const char*  title;
    uint8_t      itemCount;
    MenuItem     items[MENU_MAX_ITEMS];
};

class MenuEngine {
public:
    void init();
    void buildMenu();
    void update();
    void draw(U8G2& u8g2);

    void handleButton(ButtonEvent ev);
    void navigateBack();

    SystemState getState() const { return state; }
    uint8_t getSelectedToolId() const;

private:
    SystemState  state;
    MenuPage     pages[MENU_MAX_PAGES];
    uint8_t      pageIndex;
    uint8_t      cursorIndex;
    uint32_t     lastUpdateMs;

    // Animation values — list scroll
    float scrollY,   scrollYTarget;
    float selY,      selYTarget;
    float selW,      selWTarget;
    float sbY,       sbYTarget;
    float sbH,       sbHTarget;

    bool isAnimating() const;
    void recomputeTargets();
    void snapToTargets();
    void navigateUp();
    void navigateDown();
    void navigateConfirm();

    static const char*           getAbbrForModule(uint8_t id);
    static const unsigned char*  getIconForModule(uint8_t id);
    static const char*           getCategoryName(uint8_t catId);
    static const unsigned char*  getCatIcon(uint8_t cat);
};

extern MenuEngine menuEngine;
