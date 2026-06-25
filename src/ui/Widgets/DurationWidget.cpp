#include "DurationWidget.hpp"
#include "Display/Display.hpp"

void DurationWidget::update(float dt) {
    long h = _duration.totalHours();   // raw, unwrapped - can exceed 23
    int  mi = _duration.minutes();
    int  s  = _duration.seconds();

    if (h < 10) hourText.setText("0"+String(h));
    else hourText.setText(String(h));

    if (mi < 10) minuteText.setText("0"+String(mi));
    else minuteText.setText(String(mi));

    if (s < 10) secondText.setText("0"+String(s));
    else secondText.setText(String(s));
}

void DurationWidget::render() {
    if (!visible) {
        Disp::fillRect(x,y,_width,_height,ST77XX_BLACK);
        return;
    }

    hourText.render();
    Disp::print(":");
    minuteText.render();
    Disp::print(":");
    secondText.render();
}
