#include "DisplayEditScreen.hpp"

void DisplayEditScreen::returnToSelectMenu() {
    if (selectedIdx >= 0) {
        _displays[selectedIdx].widget.setInMode(false);
        _displays[selectedIdx].widget.setMenu(true);
    }
    mode = WidgetEditMode::SELECT;
}

void DisplayEditScreen::handleInput(physIO input) {

    switch (mode) {
        case WidgetEditMode::NONE:
            if (gridHeight.isSelected()) {
                if (input.Up.press && _rows < MAX_ROWS) _rows++;
                if (input.Down.press && _rows > MIN_ROWS) _rows--;
                if (input.Select.press) gridHeight.setSelected(false);
                break;
            }
            if (gridWidth.isSelected()) {
                if (input.Up.press && _cols < MAX_COLS) _cols++;
                if (input.Down.press && _cols > MIN_COLS) _cols--;
                if (input.Select.press) gridWidth.setSelected(false);
                break;
            }

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
                    case FocusField::Back: backWidget.handleInput(input); break;
                }
            }
            break;

        case WidgetEditMode::FOCUS:
            if (input.Up.press) {
                if (!cursorUp()) { mode = WidgetEditMode::NONE; focusField = FocusField::Cols; }
            }
            if (input.Down.press) {
                if (!cursorDown()) { mode = WidgetEditMode::NONE; focusField = FocusField::Back; }
            }
            if (input.Left.press) cursorLeft();
            if (input.Right.press) cursorRight();

            if (input.Select.press) {
                selectedIdx = itemAtCursor();
                if (selectedIdx < 0) {
                    createItemAtCursor();
                    mode = WidgetEditMode::CHANGE_TYPE;
                    _displays[selectedIdx].widget.setMode(WidgetSubMode::CHANGE_TYPE);
                    _displays[selectedIdx].widget.setInMode(true);
                } else {
                    mode = WidgetEditMode::SELECT;
                    subMode = WidgetSubMode::CHANGE_TYPE;
                    cursor.setVisible(false);
                    _displays[selectedIdx].widget.invalidate();
                    _displays[selectedIdx].widget.setMode(subMode);
                    _displays[selectedIdx].widget.setMenu(true);
                }
            }
            break;
        
        case WidgetEditMode::SELECT:
            if (input.Left.press)  { --subMode; _displays[selectedIdx].widget.setMode(subMode); }
            if (input.Right.press) { ++subMode; _displays[selectedIdx].widget.setMode(subMode); }
            if (input.Select.press) {
                switch (subMode) {
                    case WidgetSubMode::DONE:
                        _displays[selectedIdx].widget.setMenu(false);
                        selectedIdx = -1;
                        subMode = WidgetSubMode::CHANGE_TYPE;
                        mode = WidgetEditMode::FOCUS;
                        cursor.setVisible(true);
                        break;
                    case WidgetSubMode::DELETE:
                        deleteSelected();
                        subMode = WidgetSubMode::CHANGE_TYPE;
                        mode = WidgetEditMode::FOCUS;
                        cursor.setVisible(true);
                        break;
                    case WidgetSubMode::CHANGE_TYPE:
                        _displays[selectedIdx].widget.setMenu(false);
                        _displays[selectedIdx].widget.setInMode(true);
                        mode = WidgetEditMode::CHANGE_TYPE;
                        break;
                    case WidgetSubMode::MOVE:
                        _displays[selectedIdx].widget.setMenu(false);
                        _displays[selectedIdx].widget.setInMode(true);
                        mode = WidgetEditMode::MOVE;
                        break;
                    case WidgetSubMode::RESIZE:
                        _displays[selectedIdx].widget.setMenu(false);
                        _displays[selectedIdx].widget.setInMode(true);
                        mode = WidgetEditMode::RESIZE;
                        break;
                }
            }
            break;

        case WidgetEditMode::CHANGE_TYPE:
            if (selectedIdx >= 0) {
                if (input.Up.press) _displays[selectedIdx].setType(++_displays[selectedIdx].type);
                if (input.Down.press) _displays[selectedIdx].setType(--_displays[selectedIdx].type);
            }
            if (input.Select.press) returnToSelectMenu();
            break;

        case WidgetEditMode::MOVE:
            if (selectedIdx >= 0) {
                if (input.Up.press && upClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].y0 -= 1;
                    _displays[selectedIdx].y1 -= 1;
                }
                if (input.Down.press && downClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].y0 += 1;
                    _displays[selectedIdx].y1 += 1;
                }
                if (input.Left.press && leftClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].x0 -= 1;
                    _displays[selectedIdx].x1 -= 1;
                }
                if (input.Right.press && rightClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].x0 += 1;
                    _displays[selectedIdx].x1 += 1;
                }
            }
            if (input.Select.press) returnToSelectMenu();
            break;

        case WidgetEditMode::RESIZE:
            if (selectedIdx >= 0) {
                if (input.Up.press && upClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].y0 -= 1;
                }
                if (input.Down.press && downClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].y1 += 1;
                }
                if (input.Left.press && leftClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].x0 -= 1;
                }
                if (input.Right.press && rightClear(_displays[selectedIdx])) {
                    _displays[selectedIdx].x1 += 1;
                }
            }
            if (input.Select.press) returnToSelectMenu();
            break;
    }
}

bool DisplayEditScreen::isEdgeClear(const DisplayItem& target, Edge edge) {
    for (const auto& disp : _displays) {
        if (target.isNeighbour(disp) == edge) return false;
    }
    return true;
}

bool DisplayEditScreen::edgeAtBoundary(const DisplayItem& target, Edge edge) {
    switch (edge) {
        case Edge::Left:   if (target.x0 == 0) return true; break;
        case Edge::Right:  if (target.x1 == _cols) return true; break;
        case Edge::Top:    if (target.y0 == 0) return true; break;
        case Edge::Bottom: if (target.y1 == _rows) return true; break;
        default: return false;
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
        case FocusField::Cols: focusField = FocusField::Back; break;
        case FocusField::Rows: focusField = FocusField::Back; break;
        case FocusField::Grid: focusField = FocusField::Cols; break;
        case FocusField::Back: focusField = FocusField::Grid; break;
        case FocusField::Save: focusField = FocusField::Grid; break;
    }
    gridWidth.invalidate();
    gridHeight.invalidate();
}

void DisplayEditScreen::moveFocusDown() {
    switch (focusField) {
        case FocusField::Cols: focusField = FocusField::Grid; break;
        case FocusField::Rows: focusField = FocusField::Grid; break;
        case FocusField::Grid: focusField = FocusField::Back; break;
        case FocusField::Back: focusField = FocusField::Cols; break;
        case FocusField::Save: focusField = FocusField::Cols; break;
    }
    gridWidth.invalidate();
    gridHeight.invalidate();
}

void DisplayEditScreen::moveFocusLeft() {
    switch (focusField) {
        case FocusField::Cols: focusField = FocusField::Rows; break;
        case FocusField::Rows: focusField = FocusField::Cols; break;
        case FocusField::Back: focusField = FocusField::Save; break;
        case FocusField::Save: focusField = FocusField::Back; break;
        case FocusField::Grid: break;   // handled by cursor movement in FOCUS mode
    }
    gridWidth.invalidate();
    gridHeight.invalidate();
}

void DisplayEditScreen::moveFocusRight() {
    switch (focusField) {
        case FocusField::Cols: focusField = FocusField::Rows; break;
        case FocusField::Rows: focusField = FocusField::Cols; break;
        case FocusField::Back: focusField = FocusField::Save; break;
        case FocusField::Save: focusField = FocusField::Back; break;
        case FocusField::Grid: break;   // handled by cursor movement in FOCUS mode
    }
    gridWidth.invalidate();
    gridHeight.invalidate();
}

// ---- cursor movement, now living directly against _displays ----

void DisplayEditScreen::updateCursorPixelPos() {
    cursor.setSize(grid.colPitch(), grid.rowPitch());
    cursor.setPosition(
        grid.getX() + _cursorX * grid.colPitch() + grid.colPitch() / 2,
        grid.getY() + _cursorY * grid.rowPitch() + grid.rowPitch() / 2
    );
}

bool DisplayEditScreen::cursorUp() {
    int idx = itemAt(_cursorX, _cursorY);
    int nextY = (idx >= 0) ? (int)_displays[idx].y0 - 1 : (int)_cursorY - 1;
    if (nextY < 0) return false;
    _cursorY = (uint8_t)nextY;
    updateCursorPixelPos();
    return true;
}

bool DisplayEditScreen::cursorDown() {
    int idx = itemAt(_cursorX, _cursorY);
    int nextY = (idx >= 0) ? (int)_displays[idx].y1 : (int)_cursorY + 1;
    if (nextY > _rows - 1) return false;
    _cursorY = (uint8_t)nextY;
    updateCursorPixelPos();
    return true;
}

void DisplayEditScreen::cursorLeft() {
    int idx = itemAt(_cursorX, _cursorY);
    int nextX = (idx >= 0) ? (int)_displays[idx].x0 - 1 : (int)_cursorX - 1;
    if (nextX < 0) return;
    _cursorX = (uint8_t)nextX;
    updateCursorPixelPos();
}

void DisplayEditScreen::cursorRight() {
    int idx = itemAt(_cursorX, _cursorY);
    int nextX = (idx >= 0) ? (int)_displays[idx].x1 : (int)_cursorX + 1;
    if (nextX > _cols - 1) return;
    _cursorX = (uint8_t)nextX;
    updateCursorPixelPos();
}

int DisplayEditScreen::itemAt(uint8_t x, uint8_t y) {
    for (size_t i = 0; i < _displays.size(); i++) {
        if (_displays[i].contains(x, y)) return (int)i;
    }
    return -1;
}

int DisplayEditScreen::itemAtCursor() {
    return itemAt(_cursorX, _cursorY);
}

void DisplayEditScreen::createItemAtCursor() {
    DisplayWidget item;
    
    item.x0 = _cursorX;
    item.x1 = _cursorX + 1;
    item.y0 = _cursorY;
    item.y1 = _cursorY + 1;

    item.setType(TelemetryType::Speed);
    item.widget.setMode(WidgetSubMode::CHANGE_TYPE);

    _displays.push_back(item);
    selectedIdx = (int)_displays.size() - 1;
}

void DisplayEditScreen::deleteSelected() {
    if (selectedIdx < 0) return;
    _displays.erase(_displays.begin() + selectedIdx);
    grid.invalidate();
    selectedIdx = -1;
}

bool DisplayEditScreen::validateLayout()
{
    for (size_t i = 0; i < _displays.size(); ++i)
    {
        // insideGrid(cols, rows) -- the argument order is (cols, rows), and
        // this call had them reversed while onEnter() had them right. On a
        // non-square grid that validated against transposed bounds, so it
        // both accepted out-of-grid layouts and rejected valid ones. An
        // accepted bad layout is then persisted to layout.txt and reloaded
        // at boot.
        if (!_displays[i].insideGrid(_cols, _rows))
            return false;

        for (size_t j = i + 1; j < _displays.size(); ++j)
        {
            if (_displays[i].intersects(_displays[j]))
                return false;
        }
    }
    return true;
}