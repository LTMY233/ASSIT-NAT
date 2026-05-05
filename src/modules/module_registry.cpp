#include "module_registry.h"
#include "../config.h"

ModuleRegistry moduleRegistry;

void ModuleRegistry::init() {
    count = 0;
    activeIndex = 0xFF;
    currentModule = nullptr;
    for (uint8_t i = 0; i < MAX_MODULES; i++) {
        modules[i] = nullptr;
    }
}

void ModuleRegistry::registerModule(ModuleInterface* mod) {
    if (!mod || count >= MAX_MODULES) return;
    modules[count] = mod;
    mod->init();
    count++;
}

ModuleInterface* ModuleRegistry::launch(uint8_t id) {
    int8_t idx = findById(id);
    if (idx < 0) return nullptr;

    // Exit current module
    exitCurrent();

    currentModule = modules[idx];
    activeIndex = id;
    currentModule->enter();
    return currentModule;
}

void ModuleRegistry::exitCurrent() {
    if (currentModule) {
        currentModule->exit();
        currentModule = nullptr;
        activeIndex = 0xFF;
    }
}

ModuleInterface* ModuleRegistry::getModule(uint8_t id) const {
    int8_t idx = findById(id);
    return (idx >= 0) ? modules[idx] : nullptr;
}

ModuleInterface* ModuleRegistry::getByIndex(uint8_t idx) const {
    return (idx < count) ? modules[idx] : nullptr;
}

void ModuleRegistry::getByCategory(uint8_t category, ModuleInterface** list, uint8_t& outCount, uint8_t maxCount) {
    outCount = 0;
    for (uint8_t i = 0; i < count && outCount < maxCount; i++) {
        if (modules[i]->getCategory() == category) {
            list[outCount++] = modules[i];
        }
    }
}

int8_t ModuleRegistry::findById(uint8_t id) const {
    for (uint8_t i = 0; i < count; i++) {
        if (modules[i] && modules[i]->getId() == id) return i;
    }
    return -1;
}
