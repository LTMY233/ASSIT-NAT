#pragma once
#include "module_interface.h"

class ModuleRegistry {
public:
    static const uint8_t MAX_MODULES = 80;

    void init();

    // Register module (called in setup)
    void registerModule(ModuleInterface* mod);

    // Lifecycle
    ModuleInterface* launch(uint8_t id);
    void exitCurrent();
    ModuleInterface* active() const { return currentModule; }
    uint8_t activeId() const { return activeIndex; }

    // Query
    ModuleInterface* getModule(uint8_t id) const;
    uint8_t getCount() const { return count; }
    ModuleInterface* getByIndex(uint8_t idx) const;

    // Get modules by category
    void getByCategory(uint8_t category, ModuleInterface** list, uint8_t& outCount, uint8_t maxCount);

private:
    ModuleInterface* modules[MAX_MODULES];
    bool      initialized[MAX_MODULES];
    uint8_t   count;
    uint8_t   activeIndex;
    ModuleInterface* currentModule;

    int8_t findById(uint8_t id) const;
};

extern ModuleRegistry moduleRegistry;
