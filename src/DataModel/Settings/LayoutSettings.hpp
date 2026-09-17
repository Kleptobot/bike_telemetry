#ifndef LAYOUTSETTINGS_HPP
#define LAYOUTSETTINGS_HPP

#include "ISettings.hpp"
#include "JsonFile.hpp"
#include "../LayoutDataProvider.hpp"

class LayoutSettings : public ISettings {
public:
    LayoutSettings(LayoutDataProvider& provider) : _layout(provider) {}

    void save(IStorage* storage) override {
        JsonDocument doc;
        auto& data = _layout.get();

        JsonArray d = doc["displays"].to<JsonArray>();
        for (const auto& item : data.displays) {
            JsonArray disp = d.add<JsonArray>();
            disp.add(toString(item.type));
            disp.add(item.x0);
            disp.add(item.y0);
            disp.add(item.x1);
            disp.add(item.y1);
        }

        doc["rows"] = data.rows;
        doc["cols"] = data.cols;

        saveJson(storage, "/layout.txt", doc);
    }

    void load(IStorage* storage) override {
        JsonDocument doc;
        if (!loadJson(storage, "/layout.txt", doc))
            return;

        Serial.println("Found layout.txt");

        std::vector<DisplayItem> displays;
        JsonArray d = doc["displays"];
        for (const auto& member : d) {
            DisplayItem disp;
            disp.type = TelemetryTypefromString(member[0]);
            disp.x0 = member[1];
            disp.y0 = member[2];
            disp.x1 = member[3];
            disp.y1 = member[4];
            displays.push_back(disp);
        }

        // Clamp on load rather than making every consumer defend itself.
        // ArduinoJson yields 0 for a missing or unparseable key, and
        // MainScreen::onEnter divides by both of these to compute its grid
        // pitch -- so a truncated layout.txt was a divide by zero on the
        // main screen. DisplayEditScreen already clamped to 2..5; this makes
        // the guarantee hold at the point the data enters the model.
        uint8_t rows = constrain((int)(doc["rows"] | 0), LAYOUT_MIN_ROWS, LAYOUT_MAX_ROWS);
        uint8_t cols = constrain((int)(doc["cols"] | 0), LAYOUT_MIN_COLS, LAYOUT_MAX_COLS);

        _layout.update({displays, rows, cols});
    }

private:
    LayoutDataProvider& _layout;
};

#endif /* LAYOUTSETTINGS_HPP */
