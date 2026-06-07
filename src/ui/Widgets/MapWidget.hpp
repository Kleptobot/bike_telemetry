#pragma once
#include "UI/Widgets/Widget.hpp"
#include "display/Display.hpp"
#include "DataModel/DataModel.hpp"

class MapWidget : public Widget {
public:
    MapWidget(int x, int y, int w, int h, DataModel& model);

    void update(float dt) override;
    void render() override;

    void setZoom(float metersPerPixel) { _metersPerPixel = metersPerPixel; invalidate(); }
    void setCenter(double lat, double lon) { _centerLat = lat; _centerLon = lon; invalidate(); }

private:
    DataModel& _model;
    double _centerLat = 0.0;
    double _centerLon = 0.0;
    float _metersPerPixel = 2.0f; // meters represented by one pixel
    uint32_t _lastVersion = 0;
    bool _dirty = true;

    void project(double lat, double lon, int& outX, int& outY);
};
