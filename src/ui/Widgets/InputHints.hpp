#pragma once
#include "UI/Widgets/Widget.hpp"
#include "Display/Display.hpp"
#include "UI/GFX.h"
#include <Arduino.h>

// Compact input-legend row: up to three slots, each an optional 16x16 icon
// (pass nullptr for the standard "select button" glyph) plus a small text
// label. Renders at a fixed anchor the owning screen chooses -- by default
// the bottom-left free band (5,298), which is empty on every screen except
// the main screen (whose big cross occupies the right of that band).
//
// The visual language deliberately mirrors MainScreen's switch cross so the
// mapping between on-screen hints and the 5-position switch is consistent:
//   left-arrow icon  = the LEFT switch direction (usually back)
//   select glyph     = the CENTER direction (select/edit)
//   save icon        = the RIGHT direction (save)
//   UP arrow icon    = "up/down moves/adjusts"
//
// Conventions live in UIScreen.hpp; sleep (Select-held) is Main-only.
//
// No explicit erase on render: UIManager::render clears the whole canvas
// every frame, like every other widget.
class InputHintsWidget : public Widget {
public:
    // slots: how many of the three slots are actually used, so the row can
    // be narrower when a screen only needs one or two hints.
    InputHintsWidget(int x = 5, int y = 298, int slots = 3)
        : Widget(x, y, slots * SLOT_WIDTH, 16), _slotCount(slots) {}

    void setHint(int slot, const uint8_t* icon, const char* label) {
        if (slot < 0 || slot >= _slotCount) return;
        _icons[slot]  = icon;
        _labels[slot] = label;
    }

    void clearHint(int slot) {
        if (slot < 0 || slot >= _slotCount) return;
        _icons[slot]  = nullptr;
        _labels[slot] = nullptr;
    }

    void render() override {
        for (int i = 0; i < _slotCount; i++) {
            const int sx = _x + i * SLOT_WIDTH;
            if (_icons[i]) {
                Disp::drawBitmap(sx, _y, _icons[i], 16, 16, ST77XX_WHITE);
            } else if (_labels[i]) {
                // No icon given: draw the standard "select button" glyph -
                // an outlined square with a filled centre dot.
                Disp::drawRect(sx, _y, 16, 16, ST77XX_WHITE);
                Disp::fillRect(sx + 5, _y + 5, 6, 6, ST77XX_WHITE);
            } else {
                continue;
            }
            if (_labels[i]) {
                Disp::setCursor(sx + 18, _y + 4);
                Disp::setTextSize(1);
                Disp::setTextColor(ST77XX_WHITE);
                Disp::print(_labels[i]);
            }
        }
    }

private:
    static constexpr int SLOT_WIDTH = 72;
    int _slotCount;
    const uint8_t* _icons[3]  = {nullptr, nullptr, nullptr};
    const char*     _labels[3] = {nullptr, nullptr, nullptr};
};