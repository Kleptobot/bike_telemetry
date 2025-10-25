#pragma once
#include "UI/UIElement.hpp"

class ListAdapter {
public:
  virtual int getCount() const = 0;
  virtual UIElement* getItem(int index) = 0;
};