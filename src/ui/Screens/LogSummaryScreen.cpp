#include "LogSummaryScreen.hpp"
#include "HAL/HAL.hpp"
#include "Display/Display.hpp"
#include "Loggers/CsvLogParser.hpp"
#include "Loggers/FitLogParser.hpp"
#include "Loggers/TcxLogParser.hpp"
#include "UI/Screens/LogViewer.hpp"
#include <new>

// Defined here (one definition): the log path handed from LogListScreen to
// LogSummaryScreen. Declared extern in LogViewer.hpp.
String g_selectedLog;

// Stat-row labels, in refreshRows() order. Labels are padded to a fixed pixel
// width once per onEnter() (padLabel), so a refresh only ever builds the
// value column -- not the ~15-space padding loop fmtRow used to run per row
// per tick.
const char* const kRowLabels[13] = {
    "Duration", "Distance", "Laps", "Avg Speed", "Max Speed",
    "Avg HR", "Max HR", "Avg Power", "Max Power", "Avg Cadence",
    "Calories", "NP", "TSS",
};
constexpr int kRowCount = 13;

// Free heap as reported in the boot banner; used by the breadcrumbs so a
// crash report says how close to exhaustion the heap was.
uint32_t freeHeap() { return dbgHeapTotal() - dbgHeapUsed(); }

void LogSummaryScreen::onEnter() {
    _file = logViewerSelected();
    _ok = false;
    _delFailed = false;
    _subtitleCode = -1;   // force update() to push the initial subtitle
    _lastRowsMs = 0;

    // Title and date depend only on _file: set them here, once. (The title
    // widget copies its String at construction, so before this it showed the
    // file name from REGISTRATION time -- i.e. nothing -- forever.)
    titleWidget.setText(_file);
    // _file carries a leading "/"; skip it so the date shows "20260101" not
    // "/2026010".
    dateWidget.setText(_file.length() >= 9 ? _file.substring(1, 9) : _file);

    // Padded labels depend only on the font, not on the data.
    for (int i = 0; i < kRowCount; i++) _padded[i] = padLabel(kRowLabels[i]);

    openParser();

    backWidget.setVisible(true);
    deleteWidget.setVisible(true);
    hints.setHint(0, epd_bitmap_left, "list");

    // Breadcrumb: which file and how much heap. The intermittent crashes on
    // this screen were never reproducible without knowing these two.
    Serial.printf("[LogSummary] open '%s' heap=%u\n",
                  _file.c_str(), (unsigned)freeHeap());
}

bool LogSummaryScreen::openParser() {
    // Pick a parser by file extension. FitLogParser validates the ".FIT" magic
    // internally; TCX is matched by extension; anything else falls back to CSV.
    const bool isFit = (_file.length() >= 4) &&
             (memcmp(_file.c_str() + _file.length() - 4, ".FIT", 4) == 0 ||
              memcmp(_file.c_str() + _file.length() - 4, ".fit", 4) == 0);
    const bool isTcx = (_file.length() >= 4) &&
             (memcmp(_file.c_str() + _file.length() - 4, ".TCX", 4) == 0 ||
              memcmp(_file.c_str() + _file.length() - 4, ".tcx", 4) == 0);

    // nothrow new: with -fno-exceptions a failed allocation inside plain
    // new/make_unique aborts -- i.e. a factory reset, which is precisely the
    // "MCU resets when opening a log" failure mode. Report it and show the
    // Error state instead.
    LogParser* p = nullptr;
    if (isFit)      p = new (std::nothrow) FitLogParser();
    else if (isTcx) p = new (std::nothrow) TcxLogParser();
    else            p = new (std::nothrow) CsvLogParser();
    if (!p) {
        Serial.println("[LogSummary] out of memory allocating parser");
        _parser.reset();
        _parsing = false;
        _ok = false;
        return false;
    }
    _parser.reset(p);
    _parsing = _parser->open(HAL::inst().SD(), _file.c_str());
    if (!_parsing) {
        Serial.printf("[LogSummary] parser open failed for '%s' heap=%u\n",
                      _file.c_str(), (unsigned)freeHeap());
    }
    return _parsing;
}

void LogSummaryScreen::onExit() {
    // Close the log file BEFORE dropping the parser. SdFat is built with
    // DESTRUCTOR_CLOSES_FILE=0, so destroying the File32 does NOT close or
    // sync it -- "_parser.reset() closes the file" was never true on the
    // device (only the host stub's ~File32 closes, which is how this survived
    // the sim tests). An open File32 across screens keeps SdFat's cached
    // FAT/directory state alive while the host may rewrite the card over USB
    // MSC; close() is the documented, actually-happening close.
    if (_parser) _parser->close();
    _parser.reset();
    _parsing = false;
    _ok = false;
}

void LogSummaryScreen::deleteCurrentLog() {
    // Never touch the card while a ride is being logged: the logger holds an
    // open File32 for the whole ride, and mutating the FAT under it is the
    // file-corruption path. Guard here so the screen is safe even if reached
    // by an unexpected input event.
    const AppState& st = model.app();
    if (st == AppState::LOGGING || st == AppState::PAUSED) {
        Serial.println("[LogSummary] delete refused while logging");
        _delFailed = true;
        return;
    }

    // The parser holds the log open from onEnter() until onExit(). Deleting a
    // file with a live handle is at best undefined on FAT -- it frees the
    // cluster chain under the open reader -- and fails outright on Windows
    // (open files can't be unlinked). Close first: this is the precondition
    // of the remove() below.
    if (_parser) _parser->close();
    _parser.reset();
    _parsing = false;
    _ok = false;

    auto* sd = HAL::inst().SD();
    bool removed = sd && sd->remove(_file.c_str());
    Serial.printf("[LogSummary] remove('%s') -> %d mounted=%d\n",
                  _file.c_str(), (int)removed, (int)HAL::inst().SDMounted());

    // Verify the deletion landed by re-checking existence. We deliberately do
    // NOT remount: sd.end()/re-init discards SdFat's cached FAT/directory
    // sectors under any other live handle (e.g. a logger mid-ride) and is the
    // FAT-cache corruption vector. With the parser closed above, no firmware
    // handle is open here, so a plain exists() check is sufficient.
    int existsAfter = -1;   // -1 = could not verify
    if (removed) {
        existsAfter = sd->exists(_file.c_str()) ? 1 : 0;
        if (existsAfter == 1) {
            Serial.printf("[LogSummary] '%s' STILL EXISTS after remove\n",
                          _file.c_str());
            removed = false;
        }
    }
    Serial.printf("[LogSummary] remove('%s') ok=%d existsAfter=%d heap=%u\n",
                  _file.c_str(), (int)removed, existsAfter, (unsigned)freeHeap());

    if (removed) {
        emitUIEvent(UIEventType::ChangeScreen, ScreenID::LogList);
        return;
    }
    // Deletion failed (card I/O error, file already gone, verification
    // failed...). Reopen the log so the screen still shows its data instead of
    // a dead parser, and tell the user the delete did not happen. Only the
    // parser is reopened: a full onEnter() would also reset _delFailed, which
    // the subtitle keys on.
    openParser();
    _delFailed = true;
}

void LogSummaryScreen::update(float dt) {
    backWidget.setFocused(focusField == FocusField::Back);
    deleteWidget.setFocused(focusField == FocusField::Delete);

    // Subtitle is rebuilt only when the STATE changes. The old code built a
    // fresh String every loop tick (~100x/s) for a value that changes at most
    // a few times per screen visit -- pure heap churn.
    const int code = _delFailed ? 3 : _parsing ? 0 : (_ok ? 1 : 2);
    if (code != _subtitleCode) {
        _subtitleCode = code;
        subtitleWidget.setText(code == 3 ? "Delete failed"
                             : code == 0 ? "Reading..."
                             : code == 1 ? "Done" : "Error");
    }

    if (!_parsing || !_parser) return;
    for (int i = 0; _parsing && i < 16; i++) {
        _parsing = _parser->step();
    }
    _ok = !_parsing && _parser->done() && _parser->stats().valid;

    // Rebuild the stat rows at most twice a second while parsing, and always
    // on the pass where parsing finishes. The old code rebuilt ALL rows every
    // loop tick: each row ran the label-padding loop (a String copy per space
    // via getTextBounds) plus a value String plus concatenations -- on the
    // order of hundreds of malloc/free pairs of many distinct sizes per tick,
    // sustained for the whole read of a large .tcx (minutes at 96 B x 16
    // steps/tick). That sustained fragmentation is what crashed this screen
    // on big logs.
    const uint32_t now = millis();
    if (_parsing && (now - _lastRowsMs) < 500) return;
    _lastRowsMs = now;
    refreshRows();
}

void LogSummaryScreen::refreshRows() {
    if (!_parser) return;
    const RideStats& s = _parser->stats();
    durationWidget.setText(rowText(0, fmtDuration(s.durationSec)));
    distanceWidget.setText(rowText(1, fmtDistance(s.distanceM)));
    lapsWidget.setText(rowText(2, String(s.lapCount)));
    avgSpeedWidget.setText(rowText(3, fmt1(s.avgSpeedKmh) + " km/h"));
    maxSpeedWidget.setText(rowText(4, fmt1(s.maxSpeedKmh) + " km/h"));
    avgHrWidget.setText(rowText(5, fmt0(s.avgHr)));
    maxHrWidget.setText(rowText(6, fmt0(s.maxHr)));
    avgPwrWidget.setText(rowText(7, fmt0(s.avgPower) + " W"));
    maxPwrWidget.setText(rowText(8, fmt0(s.maxPower) + " W"));
    avgCadWidget.setText(rowText(9, fmt0(s.avgCadence)));
    caloriesWidget.setText(rowText(10, fmt0(s.calories) + " kcal"));
    npWidget.setText(rowText(11, fmt0(s.np) + " W"));
    tssWidget.setText(rowText(12, fmt0(s.tss) + " "));
}

void LogSummaryScreen::handleInput(const physIO input) {
    if (input.Left.press) moveFocusLeft();
    if (input.Right.press) moveFocusRight();

    switch (focusField) {
        case FocusField::Back:
            backWidget.handleInput(input);
            break;
        case FocusField::Delete:
            deleteWidget.handleInput(input);
            break;
        default: break;
    }
}

String LogSummaryScreen::padLabel(const char* label) {
    // Pad the label with spaces until it measures >= 130 px at text size 1 so
    // the value column starts at a fixed x. Called once per label per
    // onEnter() (the old fmtRow rebuilt this padding on every single row
    // update). The iteration cap makes a broken font metric produce a
    // truncated label instead of an infinite loop.
    Disp::setTextSize(1);
    String padded = label;
    int16_t x1, y1;
    uint16_t w, h;
    Disp::getTextBounds(padded, 0, 0, &x1, &y1, &w, &h);
    for (int i = 0; w < 130 && i < 64; i++) {
        padded += ' ';
        Disp::getTextBounds(padded, 0, 0, &x1, &y1, &w, &h);
    }
    return padded;
}

String LogSummaryScreen::rowText(int row, const String& value) {
    String r = _padded[row];
    r += value;
    return r;
}

String LogSummaryScreen::fmtDuration(uint32_t sec) {
    char b[16];
    snprintf(b, sizeof(b), "%u:%02u:%02u", (unsigned int)(sec / 3600),
             (unsigned int)((sec / 60) % 60), (unsigned int)(sec % 60));
    return String(b);
}

String LogSummaryScreen::fmtDistance(float m) {
    if (m >= 1000.0f) {
        char b[16];
        snprintf(b, sizeof(b), "%.2f km", m / 1000.0f);
        return String(b);
    }
    char b[16];
    snprintf(b, sizeof(b), "%.0f m", m);
    return String(b);
}

String LogSummaryScreen::fmt1(float v) {
    char b[16];
    snprintf(b, sizeof(b), "%.1f", v);
    return String(b);
}

String LogSummaryScreen::fmt0(float v) {
    char b[16];
    snprintf(b, sizeof(b), "%.0f", v);
    return String(b);
}

void LogSummaryScreen::render() {
    titleWidget.render();
    subtitleWidget.render();

    dateWidget.render();
    durationWidget.render();
    distanceWidget.render();
    lapsWidget.render();
    avgSpeedWidget.render();
    maxSpeedWidget.render();
    avgHrWidget.render();
    maxHrWidget.render();
    avgPwrWidget.render();
    maxPwrWidget.render();
    avgCadWidget.render();
    caloriesWidget.render();
    npWidget.render();
    tssWidget.render();
    
    backWidget.render();
    deleteWidget.render();
    hints.render();
}

void LogSummaryScreen::moveFocusLeft() {
    switch (focusField) {
        case FocusField::Back: focusField = FocusField::Delete; break;
        case FocusField::Delete: focusField = FocusField::Back; break;
        default: break;
    }
}

void LogSummaryScreen::moveFocusRight() {
    switch (focusField) {
        case FocusField::Back: focusField = FocusField::Delete; break;
        case FocusField::Delete: focusField = FocusField::Back; break;
        default: break;
    }
}