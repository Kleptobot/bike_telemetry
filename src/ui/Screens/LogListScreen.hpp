#pragma once
#include <vector>
#include "UI/Screens/UIScreen.hpp"
#include "UI/Screens/LogViewer.hpp"
#include "UI/Widgets/ListView.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/Widgets/InputHints.hpp"

// Enumerates saved ride logs (CSV/TCX/FIT) on the SD card, newest first.
// Selecting one hands the file to LogSummaryScreen (via logViewerSelect) and
// navigates there. A trailing "Back" row returns to the settings menu.
class LogListScreen : public UIScreen {
public:
    LogListScreen(DataModel& model);

    void onEnter() override;
    void update(float dt) override { listView.update(dt); }
    void render() override {
        listView.render();
        hints.render();
    }
    void handleInput(const physIO input) override { listView.handleInput(input); }

private:
    ListView<SelectableTextIconWidget> listView;
    InputHintsWidget hints{5, 298, 2};
    // Full device paths ("/NAME.ext") aligned with listView items; "" = Back.
    std::vector<String> _paths;
    void refreshLogList();
};