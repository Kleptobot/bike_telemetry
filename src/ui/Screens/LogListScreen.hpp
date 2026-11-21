#pragma once

#include <vector>
#include <string>
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/ListView.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/Widgets/InputHints.hpp"
#include "UI/Screens/LogViewer.hpp"

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
