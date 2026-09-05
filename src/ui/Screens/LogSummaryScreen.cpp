#include "LogSummaryScreen.hpp"
#include "HAL/HAL.hpp"
#include "Display/Display.hpp"

// Shared hand-off between LogListScreen and LogSummaryScreen (see LogViewer.hpp).
namespace {
String g_selectedLog;
}

void logViewerSelect(const char* file) { g_selectedLog = file ? file : ""; }
String logViewerSelected() { return g_selectedLog; }

void LogSummaryScreen::onEnter() {
    _file = logViewerSelected();
    // Pick a parser by file extension. FitLogParser validates the ".FIT" magic
    // internally; TCX is matched by extension; anything else falls back to CSV.
        bool isFit = (_file.length() >= 4) &&
                 (memcmp(_file.c_str() + _file.length() - 4, ".FIT", 4) == 0 ||
                  memcmp(_file.c_str() + _file.length() - 4, ".fit", 4) == 0);
        bool isTcx = (_file.length() >= 4) &&
                 (memcmp(_file.c_str() + _file.length() - 4, ".TCX", 4) == 0 ||
                  memcmp(_file.c_str() + _file.length() - 4, ".tcx", 4) == 0);
        if (isFit) _parser = std::unique_ptr<LogParser>(new FitLogParser());
        else if (isTcx) _parser = std::unique_ptr<LogParser>(new TcxLogParser());
        else            _parser = std::unique_ptr<LogParser>(new CsvLogParser());
    _parsing = _parser->open(HAL::inst().SD(), _file.c_str());
    _ok = false;
    backWidget.setVisible(true);
    deleteWidget.setVisible(true);
    hints.setHint(0, epd_bitmap_left, "list");
}

void LogSummaryScreen::update(float dt) {
    if (!_parsing) return;
    for (int i = 0; _parsing && i < 16; i++) {
        _parsing = _parser->step();
    }
    _ok = !_parsing && _parser->done() && _parser->stats().valid;

    backWidget.setFocused(focusField == FocusField::Back);
    deleteWidget.setFocused(focusField == FocusField::Delete);
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

String LogSummaryScreen::fmtDuration(uint32_t sec) {
    char b[16];
    snprintf(b, sizeof(b), "%u:%02u:%02u", sec / 3600, (sec / 60) % 60, sec % 60);
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

void LogSummaryScreen::drawRow(int y, const String& label, const String& value) {
    Disp::setTextSize(1);
    Disp::setTextColor(ST77XX_WHITE);
    Disp::setCursor(6, y);
    Disp::print(label);
    Disp::setCursor(130, y);
    Disp::print(value);
}

void LogSummaryScreen::render() {
    Disp::setTextSize(2);
    Disp::setTextColor(ST77XX_WHITE);
    Disp::setCursor(6, 6);
    String date = _file.length() >= 8 ? _file.substring(0, 8) : _file;
    Disp::print(date);

    if (!_ok) {
        Disp::setTextSize(1);
        Disp::setCursor(6, 28);
        Disp::print("Reading...");
        backWidget.render();
        deleteWidget.render();
        hints.render();
        return;
    }

        const RideStats& s = _parser->stats();
    int y = 26;
    drawRow(y, "Duration", fmtDuration(s.durationSec));        y += 12;
    drawRow(y, "Distance", fmtDistance(s.distanceM));          y += 12;
    drawRow(y, "Laps",     String(s.lapCount));                y += 12;
    drawRow(y, "AvgSpeed", fmt1(s.avgSpeedKmh) + " km/h");     y += 12;
    drawRow(y, "MaxSpeed", fmt1(s.maxSpeedKmh) + " km/h");     y += 12;
    drawRow(y, "AvgHR",    fmt0(s.avgHr));                     y += 12;
    drawRow(y, "MaxHR",    fmt0(s.maxHr));                     y += 12;
    drawRow(y, "AvgPwr",   fmt0(s.avgPower) + " W");           y += 12;
    drawRow(y, "MaxPwr",   fmt0(s.maxPower) + " W");           y += 12;
    drawRow(y, "AvgCad",   fmt0(s.avgCadence));                y += 12;
    drawRow(y, "Calories", fmt0(s.calories) + " kcal");        y += 12;
    drawRow(y, "NP",       fmt0(s.np) + " W");                 y += 12;
    drawRow(y, "TSS",      fmt1(s.tss));                       y += 12;

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