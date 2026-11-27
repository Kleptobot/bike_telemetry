#pragma once

#include <vector>
#include <string>
#include "ui/Screens/UIScreen.hpp"
#include "ui/Widgets/ListView.hpp"
#include "ui/Widgets/SelectableTextIcon.hpp"
#include "ui/Widgets/InputHints.hpp"
#include "ui/Screens/LogViewer.hpp"

// Hand-off of the user's log selection from LogListScreen to LogSummaryScreen
// lives in LogViewer.hpp (logViewerSelect / logViewerSelected).

class LogListScreen : public UIScreen {
public:
    LogListScreen(DataModel& model);

    void onEnter() override;
    void update(float dt) override;
    void render() override;
    void handleInput(const physIO input) override;

private:
    void refreshLogList();

    ListView<SelectableTextIconWidget> listView;
    std::vector<String> _paths;
    InputHintsWidget hints{5, 298, 2};
};
