#pragma once
#include "UIElement.hpp"
#include "display/Display.hpp"
#include "HAL/InputInterface.hpp"
#include "Adapters/ListAdapter.hpp"

class ListView : public UIElement {
public:
  ListView(ListAdapter* adapter, int x, int y, int width, int height)
    : adapter(adapter), x(x), y(y), width(width), height(height) {}

  void handleInput(const physIO input) {
    if (input.Down.RE)
      selected = min(selected + 1, adapter->getCount() - 1);
    if (input.Up.RE)
      selected = max(selected - 1, 0);
  }

  void render(int x, int y) override {
    int currentY = y;
    for (int i = 0; i < adapter->getCount(); ++i) {
      UIElement* item = adapter->getItem(i);
      if (!item) continue;

      // Highlight selected row
      if (i == selected)
        Disp::fillRect(0, currentY - 1, width, item->getHeight() + 2, DisplayColor::WHITE);

      // Render the item inside the rectangle
      item->render(x, y);

      currentY += item->getHeight() + spacing;
    }
  }

  int getHeight() const override { return height; }
  int getSelectedIndex() const { return selected; }

private:
  ListAdapter* adapter;
  int x, y, width, height;
  int spacing = 2;
  int selected = 0;
};