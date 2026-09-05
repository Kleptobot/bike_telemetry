#pragma once
#include <memory>
#include "UI/Screens/UIScreen.hpp"
#include "UI/Screens/LogViewer.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/Widgets/SelectableText.hpp"
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
          titleWidget(6, 6, _file),
          subtitleWidget(6, 20, "Reading..."),
          dateWidget(6, 34, "Date"),
          durationWidget(6, 48, "Duration",1),
          distanceWidget(6, 62, "Distance",1),
          lapsWidget(6, 76, "Laps",1),
          avgSpeedWidget(6, 90, "Avg Speed",1),
          maxSpeedWidget(6, 104, "Max Speed",1),
          avgHrWidget(6, 118, "Avg HR",1),
          maxHrWidget(6, 132, "Max HR",1),
          avgPwrWidget(6, 146, "Avg Power",1),
          maxPwrWidget(6, 160, "Max Power",1),
          avgCadWidget(6, 174, "Avg Cadence",1),
          caloriesWidget(6, 188, "Calories",1),
          npWidget(6, 202, "NP",1),
          tssWidget(6, 216, "TSS",1),

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
    SelectableTextWidget titleWidget;
    SelectableTextWidget subtitleWidget;

    SelectableTextWidget dateWidget;
    SelectableTextWidget durationWidget;
    SelectableTextWidget distanceWidget;
    SelectableTextWidget lapsWidget;
    SelectableTextWidget avgSpeedWidget;
    SelectableTextWidget maxSpeedWidget;
    SelectableTextWidget avgHrWidget;
    SelectableTextWidget maxHrWidget;
    SelectableTextWidget avgPwrWidget;
    SelectableTextWidget maxPwrWidget;
    SelectableTextWidget avgCadWidget;
    SelectableTextWidget caloriesWidget;
    SelectableTextWidget npWidget;
    SelectableTextWidget tssWidget;

    SelectableTextIconWidget backWidget;
    SelectableTextIconWidget deleteWidget;
    InputHintsWidget hints{5, 298, 2};
    std::unique_ptr<LogParser> _parser;
    bool _parsing = false;
    bool _ok      = false;
    String _file;

    String fmtRow(const String& label, const String& value);
    static String fmtDuration(uint32_t sec);
    static String fmtDistance(float m);
    static String fmt1(float v);
    static String fmt0(float v);

    enum class FocusField { Back, Delete };
    FocusField focusField = FocusField::Back;

    void moveFocusLeft();
    void moveFocusRight();
};