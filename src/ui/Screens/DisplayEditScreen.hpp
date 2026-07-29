#pragma once

#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/DisplayEditWidget.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/Widgets/GridOverlayWidget.hpp"
#include "UI/Widgets/CursorWidget.hpp"

enum class WidgetEditMode {
    NONE = 0,
    FOCUS,
    SELECT,
    CHANGE_TYPE,
    MOVE,
    RESIZE
};

class DisplayEditScreen : public UIScreen {
public:
    DisplayEditScreen (DataModel& model) : 
        UIScreen(model),
        gridWidthLabel{5,5,"Cols: "},
        gridWidth{gridWidthLabel.width() + 10,5,String(_cols)},

        gridHeightLabel{120,5,"Rows: "},
        gridHeight{gridHeightLabel.getX()+gridHeightLabel.width() + 5,5,String(_rows)},

        grid(32,3,2),
        cursor(0,0,32,32),
        backWidget{5,288,"Back",epd_bitmap_save},
        saveWidget{90,288,"Save",epd_bitmap_save} {
            backWidget.setOnPress([this] () {
                emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
            });
            saveWidget.setOnPress([this] () {
                if (validateLayout()) {
                    std::vector<DisplayItem> layout;
                    for (auto c : _displays)
                        layout.push_back(c);
                    this->model.layout().update( {layout, _rows, _cols} );
                    emitAppEvent({AppEventType::SaveLayout,0});
                }
                emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
            });
    }

    void onEnter() override {
        const auto& l = model.layout().get();
        _rows = constrain(l.rows, 2, 5);
        _cols = constrain(l.cols, 2, 5);
        _displays.clear();
        
        for (const auto& disp : l.displays) {
            bool noIntersections = true;
            for (const auto& other : _displays ) {
                noIntersections &= !disp.intersects(other);
            }
            if (noIntersections && disp.isValidGeometry() && disp.insideGrid(_cols, _rows)) {
                _displays.push_back(disp);
            }
        }
    }

    void update(float dt) override {
        grid.setCols(_cols);
        grid.setRows(_rows);

        gridHeight.setText(String(_rows));
        gridWidth.setText(String(_cols));

        grid.update();

        _cursorX = constrain(_cursorX, 0, _cols - 1);
        _cursorY = constrain(_cursorY, 0, _rows - 1);
        updateCursorPixelPos();

        for (auto& disp : _displays) {
            disp.setSize(grid.colPitch(), grid.rowPitch());
            disp.setPos(grid.getX(), grid.getY(), grid.colPitch(), grid.rowPitch());
        }

        gridWidth.setFocused(focusField == FocusField::Cols);
        gridHeight.setFocused(focusField == FocusField::Rows);
        cursor.setVisible(focusField == FocusField::Grid && mode == WidgetEditMode::FOCUS);
        backWidget.setFocused(focusField == FocusField::Back);
        saveWidget.setFocused(focusField == FocusField::Save);
    }

    void handleInput(physIO input) override;

    void render() override {
        gridWidthLabel.render();
        gridWidth.render();
        gridHeightLabel.render();
        gridHeight.render();
        grid.render();

        for (auto& disp : _displays) {
            disp.widget.render();
        }

        cursor.render();

        backWidget.render();
        saveWidget.render();
    }
    
private:
    struct DisplayWidget : DisplayItem {
        DisplayEditWidget   widget;

        DisplayWidget () : widget(0,0){}
        DisplayWidget (DisplayItem d) : widget(0,0) {
            setType(d.type);
            x0 = d.x0; y0 = d.y0; x1 = d.x1; y1 = d.y1;
        }

        void setSize(int colPitch, int rowPitch) {
            widget.setSize((x1-x0) * colPitch, (y1-y0) * rowPitch);
        }

        void setPos(int x, int y, int colPitch, int rowPitch) {
            widget.setPosition(x + x0 * colPitch, y + y0 * rowPitch);
        }

        void setType(TelemetryType t) { DisplayItem::type = t; widget.setType(t); }

        bool contains(int x, int y) {
            return (x0 <= x && x < x1) && (y0 <= y && y < y1);
        }
    };
    uint8_t _rows = 2, _cols = 2;

    SelectableTextWidget gridWidthLabel;
    SelectableTextWidget gridWidth;
    SelectableTextWidget gridHeightLabel;
    SelectableTextWidget gridHeight;

    GridOverlayWidget grid;
    CursorWidget cursor;
    uint8_t _cursorX = 0, _cursorY = 0;

    SelectableTextIconWidget backWidget;
    SelectableTextIconWidget saveWidget;

    enum class FocusField { Cols, Rows, Grid, Back, Save };
    FocusField focusField = FocusField::Cols;

    std::vector<DisplayWidget> _displays;

    WidgetEditMode mode = WidgetEditMode::NONE;
    WidgetSubMode subMode = WidgetSubMode::CHANGE_TYPE;

    const uint8_t MAX_ROWS = 5;
    const uint8_t MIN_ROWS = 2;
    const uint8_t MAX_COLS = 5;
    const uint8_t MIN_COLS = 2;

    int selectedIdx = -1;

    bool anySelected() {
        return  gridWidth.isSelected() ||
                gridHeight.isSelected() ||
                grid.isSelected() ||
                saveWidget.isSelected();
    }

    bool isEdgeClear(const DisplayItem& target, Edge edge);
    bool edgeAtBoundary(const DisplayItem& target, Edge edge);
    bool upClear(const DisplayItem& target);
    bool downClear(const DisplayItem& target);
    bool leftClear(const DisplayItem& target);
    bool rightClear(const DisplayItem& target);

    void moveFocusUp();
    void moveFocusDown();
    void moveFocusLeft();
    void moveFocusRight();

    // cursor movement — skips over a widget's full footprint in one step
    bool cursorUp();
    bool cursorDown();
    void cursorLeft();
    void cursorRight();
    void updateCursorPixelPos();

    int itemAt(uint8_t x, uint8_t y);
    int itemAtCursor();
    void createItemAtCursor();
    void deleteSelected();

    bool validateLayout();
    void returnToSelectMenu();
};