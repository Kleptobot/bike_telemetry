#include "LogListScreen.hpp"
#include "HAL/HAL.hpp"
#include "UI/GFX.h"

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
    StorageEntry entries[64];
    uint8_t n = 0;
    String names[64];
    uint8_t nNames = 0;
    if (HAL::inst().SD() && HAL::inst().SD()->listFiles("/", entries, 64, n)) {
        for (uint8_t i = 0; i < n && nNames < 64; i++) {
            if (!entries[i].isDir && isLogExtension(String(entries[i].name)))
                names[nNames++] = String(entries[i].name);
        }
    }

    // Insertion sort descending.
    for (uint8_t i = 1; i < nNames; i++) {
        String key = names[i];
        int j = i - 1;
        while (j >= 0 && names[j] < key) { names[j + 1] = names[j]; j--; }
        names[j + 1] = key;
    }

    for (uint8_t i = 0; i < nNames; i++) {
        listView.addItem(std::make_unique<SelectableTextIconWidget>(0, 0, names[i], epd_bitmap_save, 1, 16));
        _paths.push_back("/" + names[i]);
    }

    // Empty state still lets the user back out.
    if (nNames == 0)
        listView.addItem(std::make_unique<SelectableTextIconWidget>(0, 0, "No logs", epd_bitmap_save, 1, 16));
    listView.addItem(std::make_unique<SelectableTextIconWidget>(0, 0, "Back", epd_bitmap_left, 1, 16));
    _paths.push_back("");
    listView.setIndex(0);
}
