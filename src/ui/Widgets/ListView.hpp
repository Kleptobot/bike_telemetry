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
    }

    void removeItem(size_t index) {
        if (index < _widgets.size()) {
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
            totalHeight += w->getHeight();
        }
    }

    void render() {
        int y = _y;
        int end = std::min(_scrollOffset + _visibleCount, (int)_widgets.size());

        for (int i = _scrollOffset; i < end; ++i) {
            bool focused = (i == _selectedIndex);
            _widgets[i]->setFocused(focused);
            _widgets[i]->render(_x, y);
            y += _widgets[i]->getHeight();
        }

        if (_widgets.size() > _visibleCount) {
            float ratio = (float)_visibleCount / _widgets.size();
            int barHeight = int(ratio * totalHeight);
            int barY = int((_scrollOffset / float(_widgets.size())) * totalHeight);
            Disp::drawRect(SCREEN_WIDTH - 4, barY, 2, barHeight, DispCol::WHITE);
        }
    }

    void onItemSelected(ItemSelectedCallback cb) { _onSelected = std::move(cb); }

    WidgetT* getSelectedItem() {
        return (_selectedIndex >= 0 && _selectedIndex < _widgets.size())
            ? _widgets[_selectedIndex].get()
            : nullptr;
    }

    size_t size() const { return _widgets.size(); }

private:
    int _x, _y;
    int _selectedIndex = 0;
    int _scrollOffset = 0;
    int _visibleCount;
    int totalHeight=0;
    std::vector<std::unique_ptr<WidgetT>> _widgets;
    ItemSelectedCallback _onSelected;

    void moveSelection(int delta) {
        if (_widgets.empty()) return;
        _selectedIndex = (_selectedIndex + delta + _widgets.size()) % _widgets.size();
        clampScroll();
    }

    void clampScroll() {
        if (_selectedIndex < _scrollOffset)
            _scrollOffset = _selectedIndex;
        else if (_selectedIndex >= _scrollOffset + _visibleCount)
            _scrollOffset = _selectedIndex - _visibleCount + 1;
    }

    void selectItem() {
        if (_onSelected && !_widgets.empty())
            _onSelected(*_widgets[_selectedIndex]);
    }
};