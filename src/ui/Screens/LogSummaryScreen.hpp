#pragma once
#include <memory>
#include "UI/Screens/UIScreen.hpp"
#include "UI/Screens/LogViewer.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/Widgets/InputHints.hpp"
#include "Loggers/LogSummary.hpp"
#include "HAL/HAL.hpp"

// Summary of one saved ride, read and parsed from the log the user picked on
// LogListScreen (handed over via logViewerSelect/logViewerSelected). Parsing
// runs incrementally in update() across frames so a large CSV never blocks the
// UI loop; the metrics render once parsing completes.
class LogSummaryScreen : public UIScreen {
public:
    LogSummaryScreen(DataModel& model)
        : UIScreen(model),
          backWidget(5, 280, "Back", epd_bitmap_left),
          deleteWidget(130, 280, "Del", epd_bitmap_left) {
        backWidget.setOnPress([this]() {
            emitUIEvent(UIEventType::ChangeScreen, ScreenID::LogList);
        });
        deleteWidget.setOnPress([this]() {
            if (HAL::inst().SD()->remove(_file.c_str())) {
                emitUIEvent(UIEventType::ChangeScreen, ScreenID::LogList);
            }
        });
    }

    void onEnter() override;
    void update(float dt) override;
    void render() override;
    void handleInput(const physIO input) override;

private:
    SelectableTextIconWidget backWidget;
    SelectableTextIconWidget deleteWidget;
    InputHintsWidget hints{5, 298, 2};
    std::unique_ptr<LogParser> _parser;
    bool _parsing = false;
    bool _ok      = false;
    String _file;

    void drawRow(int y, const String& label, const String& value);
    static String fmtDuration(uint32_t sec);
    static String fmtDistance(float m);
    static String fmt1(float v);
    static String fmt0(float v);

    enum class FocusField { Back, Delete };
    FocusField focusField = FocusField::Back;

    void moveFocusLeft();
    void moveFocusRight();
};