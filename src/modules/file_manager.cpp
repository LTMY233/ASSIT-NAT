#include "file_manager.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createFileManager() { return new FileManager(); }

void FileManager::init() {
    memset(fileNames, 0, sizeof(fileNames));
    memset(fileSizes, 0, sizeof(fileSizes));
    fileCount = 0; selectedIdx = 0;
    confirmDelete = false; showInfo = false;
    infoText[0] = '\0';
}

void FileManager::enter() {
    selectedIdx = 0; confirmDelete = false;
    showInfo = false; infoText[0] = '\0';
    scanFiles();
    displayMgr.setDirty();
}

void FileManager::exit() {
    confirmDelete = false; showInfo = false;
}

void FileManager::scanFiles() {
    fileCount = 0;
    Dir dir = LittleFS.openDir("/");
    while (dir.next() && fileCount < FM_MAX_FILES) {
        strCopySafe(fileNames[fileCount], dir.fileName().c_str(), 32);
        fileSizes[fileCount] = dir.fileSize();
        fileCount++;
    }
}

void FileManager::deleteSelected() {
    if (selectedIdx >= fileCount) return;
    String path = "/";
    path += fileNames[selectedIdx];
    if (LittleFS.remove(path)) {
        snprintf(infoText, sizeof(infoText), "Deleted: %s", fileNames[selectedIdx]);
    } else {
        snprintf(infoText, sizeof(infoText), "Delete failed");
    }
    scanFiles();
    if (selectedIdx >= fileCount && fileCount > 0) selectedIdx = fileCount - 1;
    confirmDelete = false;
    showInfo = true;
    displayMgr.setDirty();
}

void FileManager::update() {}

void FileManager::handleButton(ButtonEvent ev) {
    if (confirmDelete) {
        if (ev == BTN_OK_SHORT) {
            deleteSelected();
        } else if (ev == BTN_UP_SHORT || ev == BTN_DOWN_SHORT) {
            confirmDelete = false;
            displayMgr.setDirty();
        }
        return;
    }

    if (showInfo) {
        if (ev != BTN_NONE) {
            showInfo = false;
            displayMgr.setDirty();
        }
        return;
    }

    switch (ev) {
        case BTN_UP_SHORT:
            if (selectedIdx > 0) selectedIdx--;
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (selectedIdx + 1 < fileCount) selectedIdx++;
            displayMgr.setDirty();
            break;
        case BTN_OK_SHORT:
            if (fileCount > 0) confirmDelete = true;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void FileManager::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "File Manager");

    if (fileCount == 0) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(2, 30, "No files found");
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(0, 63, "OK=refresh");
        return;
    }

    if (showInfo) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(2, 30, infoText);
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(0, 63, "Any key=back");
        return;
    }

    if (confirmDelete) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(2, 28, "Delete this file?");
        u8g2.drawStr(2, 36, fileNames[selectedIdx]);
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(0, 63, "OK=confirm UP/DN=cancel");
        return;
    }

    // File list
    uint8_t y = 24;
    for (uint8_t i = 0; i < fileCount && y < 56; i++) {
        u8g2.setFont(FONT_BODY);
        char buf[48];
        uint32_t sz = fileSizes[i];
        const char* unit = "B";
        if (sz >= 1024) { sz /= 1024; unit = "KB"; }

        snprintf(buf, sizeof(buf), "%c %s %u%s",
                 (i == selectedIdx) ? '>' : ' ',
                 fileNames[i], (unsigned int)sz, unit);
        u8g2.drawStr(2, y, buf);
        y += 10;
    }

    u8g2.setFont(FONT_DATA);
    snprintf(infoText, sizeof(infoText), "%u files OK=del",
             fileCount);
    u8g2.drawStr(0, 63, infoText);
}
