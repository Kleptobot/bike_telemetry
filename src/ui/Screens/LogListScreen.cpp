#include "LogListScreen.hpp"
#include "HAL/HAL.hpp"
#include "UI/GFX.h"
#include "UI/Screens/LogViewer.hpp"
#include <new>

LogListScreen::LogListScreen(DataModel& model)
    : UIScreen(model),
      listView(5, 5, 9) {
    listView.onItemSelected([this](SelectableTextIconWidget&) {
        const int i = listView.selectedIndex();
        if (i < 0) return;
        // When no logs are present, _paths has 1 element ("") from the "Back" entry
        // and "No logs" is at index 0 without a corresponding _paths entry
        if (_paths.empty() || (_paths.size() == 1 && _paths[0].length() == 0)) {
            // No logs present - both widgets should go to SettingsMenu
            emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
            return;
        }
        if (i >= (int)_paths.size() + 1) return;
        if (_paths[i].length() == 0) {
            emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
        } else {
            logViewerSelect(_paths[i].c_str());
            emitUIEvent(UIEventType::ChangeScreen, ScreenID::LogSummary);
        }
    });
}

static bool endsWith(const String& s, const char* suf) {
    size_t n = strlen(suf);
    return s.length() >= n && strncmp(s.c_str() + s.length() - n, suf, n) == 0;
}

static bool isLogExtension(const String& name) {
    return endsWith(name, ".csv") || endsWith(name, ".TCX") || endsWith(name, ".tcx")
        || endsWith(name, ".FIT") || endsWith(name, ".fit");
}

void LogListScreen::onEnter() {
    // Listing is a read-only directory scan and is safe even while a ride is
    // being logged (the logger holds an open File32, but reading the directory
    // does not mutate the FAT). Only deletion is gated on idle, in
    // LogSummaryScreen::deleteCurrentLog().
    //
    // Remounting is deliberately NOT done here: sd.end()/re-init discards
    // cached FAT/dir sectors under any live logger handle and is the FAT-cache
    // corruption vector. No firmware file handle is open while this screen is
    // active, so a plain listFiles read is sufficient.
    refreshLogList();
    hints.setHint(0, epd_bitmap_UP, "move");
    hints.setHint(1, nullptr, "view");
}

void LogListScreen::refreshLogList() {
    listView.clear();
    _paths.clear();

    // Collect matching logs (up to a small cap) then sort newest-first. The
    // filenames are timestamped "YYYYMMDD_HHMMSS.ext", so a descending string
    // compare is a descending-time sort.
    //
    // STACK: the loop() task only has a 4 KB stack (nRF52 core
    // LOOP_STACK_SZ = 256*4 words). A stack-resident StorageEntry[64] (4.3 KB)
    // plus a String[64] (~2 KB) overflowed it on every call here -- the crash
    // when navigating around the Logs screens. Both now live on the heap:
    // entries is static (single-threaded scratch), names is a vector sized to
    // the actual entry count.
    static StorageEntry entries[64];
    uint8_t n = 0;
    std::vector<String> names;
    names.reserve(64);
    if (HAL::inst().SD() && HAL::inst().SD()->listFiles("/", entries, 64, n)) {
        Serial.printf("[LogList] listed %u entries\n", (unsigned)n);
        for (uint8_t i = 0; i < n; i++) {
            Serial.printf("[LogList] entry[%u]: name='%s' isDir=%d isLog=%d\n", i, entries[i].name, (int)entries[i].isDir, isLogExtension(String(entries[i].name)));
            if (isLogExtension(String(entries[i].name)))
                names.push_back(String(entries[i].name));
        }
    } else {
        Serial.println("[LogList] listFiles failed (unmounted or no card)");
    }

    // Insertion sort descending.
    for (size_t i = 1; i < names.size(); i++) {
        String key = names[i];
        int j = i - 1;
        while (j >= 0 && names[j] < key) { names[j + 1] = names[j]; j--; }
        names[j + 1] = key;
    }

    // nothrow new + null checks: with -fno-exceptions a failed allocation
    // inside make_unique aborts -- i.e. "the MCU resets when navigating the
    // Logs screens", exactly the failure mode being hardened against. A
    // truncated list is recoverable; a factory reset is not.
    auto addItem = [this](const String& text, const uint8_t* icon) -> bool {
        SelectableTextIconWidget* w =
            new (std::nothrow) SelectableTextIconWidget(0, 0, text, icon, 1, 16);
        if (!w) return false;
        listView.addItem(std::unique_ptr<SelectableTextIconWidget>(w));
        return true;
    };

    for (size_t i = 0; i < names.size(); i++) {
        if (!addItem(names[i], epd_bitmap_save)) {
            Serial.println("[LogList] out of memory; list truncated");
            break;
        }
        _paths.push_back("/" + names[i]);
    }

    // Empty state still lets the user back out.
    if (names.empty()) addItem("No logs", epd_bitmap_save);
    // Selection is bounds-checked against _paths.size(), so if even the Back
    // row's widget cannot be allocated the screen degrades (no exit) instead
    // of resetting the device.
    if (addItem("Back", epd_bitmap_left)) _paths.push_back("");
    listView.setIndex(0);
}

void LogListScreen::update(float dt) {
    listView.update(dt);
}

void LogListScreen::render() {
    listView.render();
    hints.render();
}

void LogListScreen::handleInput(const physIO input) {
    listView.handleInput(input);
}
