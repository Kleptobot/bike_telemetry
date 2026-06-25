#pragma once
#include "UI/Widgets/Widget.hpp"
#include "SelectableText.hpp"
#include "DataModel/TimeDataProvider.hpp"

// Read-only display of a timeDuration as H:MM:SS, e.g. for elapsed lap time.
// Unlike TimeWidget, this never edits the underlying value - hours are shown
// as a raw, unwrapped count (timeDuration::totalHours()), so durations over
// 24h display as e.g. "27:14:05" rather than wrapping into a days field.
class DurationWidget : public Widget {
public:
    DurationWidget(int x, int y, const timeDuration& duration)
        : Widget(x, y),
          _duration(duration),
          hourText(x, y, ""),
          minuteText(x + 12*3, y, ""),
          secondText(x + 12*6, y, "") {
            _width = 12*10;
            _height = 16;
          }

    void render() override;
    void update(float dt) override;

    // Read-only: no handleInput, no isSelected/focus state - this widget
    // never participates in input focus navigation.

private:
    const timeDuration& _duration;

    SelectableTextWidget hourText;
    SelectableTextWidget minuteText;
    SelectableTextWidget secondText;
};
