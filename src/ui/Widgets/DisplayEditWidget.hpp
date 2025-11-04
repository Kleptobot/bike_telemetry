#pragma once
#include "UI/Widgets/Widget.hpp"
#include "SelectableText.hpp"
#include "Display/Display.hpp"
#include "DataModel.hpp"
#include "UI/GFX.h"

class DisplayEditWidget : public SelectableTextWidget {
    public:
        DisplayEditWidget(int x, int y)
            : SelectableTextWidget(x, y, "") {}

        
    void handleInput(physIO input) override {
        if (input.Select.press) selected = !selected;
        else if (selected) {
            if (input.Up.press) --_type;
            else if (input.Down.press) ++_type;
        }
    }

    void update(float dt) override {
        setText(toString(_type));
    }

    void setType(TelemetryType type) { _type = type; }
    const TelemetryType& type() const { return _type; }

    private:
        TelemetryType _type;
};