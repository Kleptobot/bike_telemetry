#ifndef TELEMETRY_PUBLISHER_H
#define TELEMETRY_PUBLISHER_H

#include "../DataModel/DataModel.hpp"
#include "../Fusion/Fusion.hpp"

class TelemetryPublisher {
public:
    void begin(DataModel& model);
    void update();
    void resetDistance();

private:
    DataModel* _model = nullptr;
    FusionEngine _fusion;
    bool _prevGpsPosValid = false;
};

#endif /* TELEMETRY_PUBLISHER_H */
