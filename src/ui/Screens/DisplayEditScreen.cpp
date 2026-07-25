#include "DisplayEditScreen.hpp"


void DisplayEditScreen::handleInput(physIO input) {

    switch (mode) {
        case WidgetEditMode::NONE:
            if (gridHeight.isSelected()) {
                if (input.Up.press && _rows < MAX_ROWS) _rows++;
                if (input.Down.press && _rows > MIN_ROWS) _rows--;
                if (input.Select.press) gridHeight.setSelected(false);   // confirm, drop back to browsing
                break;
            }
            if (gridWidth.isSelected()) {
                if (input.Up.press && _cols < MAX_COLS) _cols++;
                if (input.Down.press && _cols > MIN_COLS) _cols--;
                if (input.Select.press) gridWidth.setSelected(false);
                break;
            }

            // not currently adjusting a field — browsing the 4 top-level targets
            if (input.Up.press) moveFocusUp();
            if (input.Down.press) moveFocusDown();
            if (input.Left.press) moveFocusLeft();
            if (input.Right.press) moveFocusRight();
            if (input.Select.press) {
                switch (focusField) {
                    case FocusField::Rows: gridHeight.setSelected(true); break;
                    case FocusField::Cols: gridWidth.setSelected(true); break;
                    case FocusField::Grid: mode = WidgetEditMode::FOCUS; grid.setSelected(true); break;
                    case FocusField::Save: saveWidget.handleInput(input); break;
                }
            }
            break;

        case WidgetEditMode::FOCUS:
            if (input.Up.press) {
                if (!grid.cursorUp()) { mode = WidgetEditMode::NONE; focusField = FocusField::Cols;}
            }
            if (input.Down.press) {
                if(!grid.cursorDown()) {mode = WidgetEditMode::NONE; focusField = FocusField::Save;}
            }
            if (input.Left.press) grid.cursorLeft();
            if (input.Right.press) grid.cursorRight();

            if (input.Select.press) {
                selectItemAtCursor();   // sets selectedIdx (or selected ptr)
                if (selectedIdx < 0) {
                    createItemAtCursor();          // empty cell — spawn a new 1x1 DisplayItem here
                    mode = WidgetEditMode::CHANGE_TYPE;  // new widget needs a type before anything else is meaningful
                    _displays[selectedIdx].widget.setMode(WidgetSubMode::CHANGE_TYPE);
                } else {
                    mode = WidgetEditMode::SELECT;
                    _displays[selectedIdx].widget.setMode(subMode);
                }
            }
            break;
        
        case WidgetEditMode::SELECT:
            if (input.Left.press)  --subMode;
            if (input.Right.press) ++subMode;
            if (input.Select.press) {
                switch (subMode) {
                    case WidgetSubMode::DONE:
                        selectedIdx = -1;
                        mode = WidgetEditMode::FOCUS;
                        break;
                    case WidgetSubMode::DELETE:
                        deleteSelected();
                        mode = WidgetEditMode::FOCUS;   // deletion implies exit — nothing left to act on
                        break;
                    case WidgetSubMode::CHANGE_TYPE: mode = WidgetEditMode::CHANGE_TYPE; break;
                    case WidgetSubMode::MOVE:        mode = WidgetEditMode::MOVE; break;
                    case WidgetSubMode::RESIZE:      mode = WidgetEditMode::RESIZE; break;
                }
            }
            _displays[selectedIdx].widget.setMode(subMode); 
            break;

        case WidgetEditMode::CHANGE_TYPE:
            if (selectedIdx >= 0) {
                if (input.Up.press) _displays[selectedIdx].setType(++_displays[selectedIdx].type);
                if (input.Down.press) _displays[selectedIdx].setType(--_displays[selectedIdx].type);
            }
            if (input.Select.press) {
                mode = WidgetEditMode::SELECT;
            }
            break;

        case WidgetEditMode::MOVE:
            if (selectedIdx >= 0) {
                if (input.Up.press && upClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].y0 -= 1;  //move both y coords up by the pitch amount
                    _displays[selectedIdx].y1 -= 1;
                }
                if (input.Down.press && downClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].y0 += 1;  //move both y coords down by the pitch amount
                    _displays[selectedIdx].y1 += 1;
                }
                if (input.Left.press && leftClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].x0 -= 1;  //move both x coords left by the pitch amount
                    _displays[selectedIdx].x1 -= 1;
                }
                if (input.Right.press && rightClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].x0 += 1;  //move both x coords right by the pitch amount
                    _displays[selectedIdx].x1 += 1;
                }
            }
            if (input.Select.press) {
                mode = WidgetEditMode::SELECT;
            }
            break;

        case WidgetEditMode::RESIZE:
            if (selectedIdx >= 0) {
                if (input.Up.press && upClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].y0 -= 1;  //move only y0 up by the pitch amount
                }
                if (input.Down.press && downClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].y1 += 1;  //move only y1 down by the pitch amount
                }
                if (input.Left.press && leftClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].x0 -= 1;  //move only x0 left by the pitch amount
                }
                if (input.Right.press && rightClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].x1 += 1;  //move only x1 right by the pitch amount
                }
            }
            if (input.Select.press) {
                mode = WidgetEditMode::SELECT;
            }
            break;
    }
}

bool DisplayEditScreen::isEdgeClear(const DisplayItem& target, Edge edge) {
    for (const auto& disp : _displays) {
        if (target.isNeighbour(disp) == edge) return false; // The edge is blocked by 'box'
    }
    return true; // No displays found on this edge
}

bool DisplayEditScreen::edgeAtBoundary(const DisplayItem& target, Edge edge) {
    switch (edge) {
        case Edge::Left:
            if (target.x0 == 0) return true;
            break;
        
        case Edge::Right:
            if (target.x1 == _cols) return true;
            break;
        
        case Edge::Top:
            if (target.y0 == 0) return true;
            break;
        
        case Edge::Bottom:
            if (target.y1 == _rows) return true;
            break;

        default:
            return false;
            break;
    }
    return false;
}


bool DisplayEditScreen::upClear(const DisplayItem& target) {
    return isEdgeClear(target, Edge::Top) && !edgeAtBoundary(target,Edge::Top);
}

bool DisplayEditScreen::downClear(const DisplayItem& target) {
    return isEdgeClear(target, Edge::Bottom) && !edgeAtBoundary(target,Edge::Bottom);
}

bool DisplayEditScreen::leftClear(const DisplayItem& target) {
    return isEdgeClear(target, Edge::Left) && !edgeAtBoundary(target,Edge::Left);
}

bool DisplayEditScreen::rightClear(const DisplayItem& target) {
    return isEdgeClear(target, Edge::Right) && !edgeAtBoundary(target,Edge::Right);
}

void DisplayEditScreen::moveFocusUp() {
    switch (focusField) {
        case FocusField::Cols: focusField = FocusField::Save; break;
        case FocusField::Rows: focusField = FocusField::Save; break;
        case FocusField::Grid: focusField = FocusField::Cols; break;
        case FocusField::Save: focusField = FocusField::Grid; break;
    }
}

void DisplayEditScreen::moveFocusDown() {
    switch (focusField) {
        case FocusField::Cols: focusField = FocusField::Grid; break;
        case FocusField::Rows: focusField = FocusField::Grid; break;
        case FocusField::Grid: focusField = FocusField::Save; break;
        case FocusField::Save: focusField = FocusField::Cols; break;
    }
}

void DisplayEditScreen::moveFocusLeft() {
    switch (focusField) {
        case FocusField::Cols: focusField = FocusField::Rows; break;
        case FocusField::Rows: focusField = FocusField::Cols; break;
    }
}

void DisplayEditScreen::moveFocusRight() {
    switch (focusField) {
        case FocusField::Cols: focusField = FocusField::Rows; break;
        case FocusField::Rows: focusField = FocusField::Cols; break;
    }
}

void DisplayEditScreen::selectItemAtCursor() {
    selectedIdx = -1;
    for (size_t i = 0; i < _displays.size(); i++) {
        if (_displays[i].topLeft(grid.cursorX(), grid.cursorY())) {   // whatever your DisplayItem's cell-containment check is
            selectedIdx = (int)i;
            return;
        }
    }
}

void DisplayEditScreen::createItemAtCursor() {
    DisplayWidget item;
    
    item.x0 = grid.cursorX();
    item.x1 = grid.cursorX() + 1;
    item.y0 = grid.cursorY();
    item.y1 = grid.cursorY() + 1;

    item.setType(TelemetryType::Speed);   // whatever your "unset" sentinel is
    item.widget.setMode(WidgetSubMode::CHANGE_TYPE);

    _displays.push_back(item);
    selectedIdx = (int)_displays.size() - 1;
}

void DisplayEditScreen::deleteSelected() {
    if (selectedIdx < 0) return;
    _displays.erase(_displays.begin() + selectedIdx);
    selectedIdx = -1;
}