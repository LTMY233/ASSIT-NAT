#pragma once
#include "module_interface.h"
#include <LittleFS.h>

#define FM_MAX_FILES 32

class FileManager : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 7; }
    const char* getName() const override     { return "File Manager"; }
    const char* getTitle() const override    { return "File Manager"; }
    uint8_t     getId() const override       { return 81; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    char     fileNames[FM_MAX_FILES][32];
    uint32_t fileSizes[FM_MAX_FILES];
    uint8_t  fileCount;
    uint8_t  selectedIdx;
    bool     confirmDelete;
    bool     showInfo;
    char     infoText[48];

    void scanFiles();
    void deleteSelected();
};

ModuleInterface* createFileManager();
