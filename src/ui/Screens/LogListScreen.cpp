#include "LogListScreen.hpp"
#include "HAL/HAL.hpp"
#include "UI/GFX.h"
#include <new>

LogListScreen::LogListScreen(DataModel& model)
    : UIScreen(model),
      listView(5, 5, 9) {
    listView.onItemSelected([this](SelectableTextIconWidget&) {
        const int i = listView.selectedIndex();
        if (i < 0 || i >= (int)_paths.size()) return;
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
    // The card's contents may have changed behind SdFat's back: the user can
    // pull the card, delete/add logs on a PC and re-insert it. SdFat serves
    // directory listings from its cached FAT/dir sectors, so without this the
    // list would still show entries deleted on the PC (and miss new ones).
    // MSC is guaranteed paused/drained on this screen, and no firmware file
    // handles are open here, so remounting is safe.
    bool remounted = HAL::inst().SD()->remount();
    Serial.printf("[LogList] remount=%d\n", (int)remounted);
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
            if (!entries[i].isDir && isLogExtension(String(entries[i].name)))
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
