#pragma once
#include <vector>
#include <memory>
#include <functional>
#include "HAL/InputInterface.hpp"
#include "display/Display.hpp"

template<typename WidgetT>
class ListView {
public:
    using ItemSelectedCallback = std::function<void(WidgetT&)>;

    ListView(int x, int y, int visibleCount = 4) : _x(x), _y(y), _visibleCount(visibleCount) {}

    void addItem(std::unique_ptr<WidgetT> widget) {
        _widgets.push_back(std::move(widget));
        totalHeight += (_widgets.back()->height() + margin);
    }

    void removeItem(size_t index) {
        if (index < _widgets.size()) {
            totalHeight += (_widgets[index]->height() + margin);
            _widgets.erase(_widgets.begin() + index);
            if (_selectedIndex >= _widgets.size())
                _selectedIndex = _widgets.empty() ? 0 : _widgets.size() - 1;
            clampScroll();
        }
    }

    void clear() {
        _widgets.clear();
        _selectedIndex = 0;
        _scrollOffset = 0;
    }

    void handleInput(const physIO& input) {
        if (input.Up.press) moveSelection(-1);
        else if (input.Down.press) moveSelection(+1);
        else if (input.Select.press) selectItem();
    }

    void update(float dt) {
        totalHeight = 0;
        for (auto& w : _widgets) {
            w->update(dt);
            totalHeight += (w->height() + margin);
        }
    }

    void render() {
        int y = _y;
        int end = std::min(_scrollOffset + _visibleCount, (int)_widgets.size());

        for (int i = _scrollOffset; i < end; ++i) {
            bool focused = (i == _selectedIndex);
            _widgets[i]->setFocused(focused);
            _widgets[i]->render(_x, y);
            y += _widgets[i]->height() + margin;
        }

        float ratio = _visibleCount /  float(_widgets.size());
        barHeight = int(totalHeight / _widgets.size()) * ratio;
        barY = int((_selectedIndex / float(_widgets.size())) * totalHeight * ratio);
        Disp::fillRect(SCREEN_WIDTH - 4, barY+_y, 4, barHeight, ST77XX_WHITE);
    }

    int selectedIndex() const { return _selectedIndex; }

    void onItemSelected(ItemSelectedCallback cb) { _onSelected = std::move(cb); }

    WidgetT* getSelectedItem() {
        return (_selectedIndex >= 0 && _selectedIndex < _widgets.size())
            ? _widgets[_selectedIndex].get()
            : nullptr;
    }

    size_t size() const { return _widgets.size(); }

    void invalidate() {
        int w=0,h=0;
        int end = std::min(_scrollOffset + _visibleCount, (int)_widgets.size());
        for (int i = _scrollOffset; i<end; i++){
            h += (_widgets[i].get()->height() + margin);
            if (_widgets[i].get()->width() > w) w = _widgets[i].get()->width();
            _widgets[i].get()->invalidate();
        }
        Disp::markDirty(SCREEN_WIDTH - 4,_y,4,totalHeight);
    }

private:
    int _x, _y;
    int _selectedIndex = 0;
    int _scrollOffset = 0;
    uint16_t _visibleCount;
    int totalHeight=0;
    int margin = 5;
    int barHeight = 0, barY = 0;
    std::vector<std::unique_ptr<WidgetT>> _widgets;
    ItemSelectedCallback _onSelected;

    void moveSelection(int delta) {
        if (_widgets.empty()) return;
        invalidate();
        _selectedIndex = (_selectedIndex + delta + _widgets.size()) % _widgets.size();
        clampScroll();
        invalidate();
    }

    void clampScroll() {
        if (_selectedIndex < _scrollOffset) {
            _scrollOffset = _selectedIndex;
        }
        else if (_selectedIndex >= _scrollOffset + _visibleCount) {
            _scrollOffset = _selectedIndex - _visibleCount + 1;
        }
    }

    void selectItem() {
        if (_onSelected && !_widgets.empty())
            _onSelected(*_widgets[_selectedIndex]);
    }
};