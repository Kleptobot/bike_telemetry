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
    backWidget.setFocused(focusField == FocusField::Back);
    deleteWidget.setFocused(focusField == FocusField::Delete);

    subtitleWidget.setText(_parsing ? "Reading..." : (_ok ? "Done" : "Error"));
    dateWidget.setText(_file.length() >= 8 ? _file.substring(0, 8) : _file);


    if (!_parsing) return;
    for (int i = 0; _parsing && i < 16; i++) {
        _parsing = _parser->step();
    }
    _ok = !_parsing && _parser->done() && _parser->stats().valid;
    durationWidget.setText(fmtRow("Duration", fmtDuration(_parser->stats().durationSec)));
    distanceWidget.setText(fmtRow("Distance", fmtDistance(_parser->stats().distanceM)));
    lapsWidget.setText(fmtRow("Laps", String(_parser->stats().lapCount)));
    avgSpeedWidget.setText(fmtRow("Avg Speed", fmt1(_parser->stats().avgSpeedKmh) + " km/h"));
    maxSpeedWidget.setText(fmtRow("Max Speed", fmt1(_parser->stats().maxSpeedKmh) + " km/h"));
    avgHrWidget.setText(fmtRow("Avg HR", fmt0(_parser->stats().avgHr)));
    maxHrWidget.setText(fmtRow("Max HR", fmt0(_parser->stats().maxHr)));
    avgPwrWidget.setText(fmtRow("Avg Power", fmt0(_parser->stats().avgPower) + " W"));
    maxPwrWidget.setText(fmtRow("Max Power", fmt0(_parser->stats().maxPower) + " W"));
    avgCadWidget.setText(fmtRow("Avg Cadence", fmt0(_parser->stats().avgCadence)));
    caloriesWidget.setText(fmtRow("Calories", fmt0(_parser->stats().calories) + " kcal"));
    npWidget.setText(fmtRow("NP", fmt0(_parser->stats().np) + " W"));
    tssWidget.setText(fmtRow("TSS", fmt0(_parser->stats().tss) + " "));
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

String LogSummaryScreen::fmtRow(const String& label, const String& value) {
    //make sure the second string is 130 px after the start of the first string, so the two columns are aligned
    //then return the whole string with padding so it can be given to a widget
    int16_t x1,y1;
    uint16_t w,h;
    Disp::setTextSize(1);
    String paddedValue = label;
    Disp::getTextBounds(paddedValue, 0, 0, &x1, &y1, &w, &h);
    while (w < 130) {
        paddedValue += " ";
        Disp::getTextBounds(paddedValue, 0, 0, &x1, &y1, &w, &h);
    }
    return paddedValue + value;
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