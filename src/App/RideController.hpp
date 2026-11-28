#ifndef RIDE_CONTROLLER_H
#define RIDE_CONTROLLER_H

#include <stdint.h>

#include "../AppEvents.hpp"
#include "../Loggers/ILogger.hpp"
#include "../Loggers/FITLogger.hpp"
#include "../Loggers/TCXLogger.hpp"
#include "../Loggers/CSVLogger.hpp"
#include "../Fusion/FtpEstimator.hpp"
#include "SettingsStore.hpp"
#include "TelemetryPublisher.hpp"

enum class AutoPauseAction {
    None,
    Pause,
    Resume
};

class RideController {
public:
    void begin(IStorage* storage, DataModel& model,
               TelemetryPublisher& telemetry, SettingsStore& settings);
    void configureLogger();

    bool update(AppState state, AppState previousState);
    void updateTimers(float dt, AppState state);

    void start(AppState& state);
    void stop(AppState& state);
    void pause(AppState& state);
    void resume(AppState& state);
    bool newLap(AppState state);

    bool shouldAutoLap(AppState state);
    AutoPauseAction updateAutoPause(AppState state, uint32_t now);
    void resetLapAnchors();

    uint16_t suggestedFtp() const { return _ftpEstimator.suggestedFtp(); }
    uint16_t best20MinAvg() const { return _ftpEstimator.best20MinAvg(); }
    bool ftpHasQualifyingWindow() const { return _ftpEstimator.hasQualifyingWindow(); }

    float elapsedSec() const { return _rideElapsedSec; }
    float movingSec() const { return _rideMovingSec; }

private:
    IStorage* _storage = nullptr;
    DataModel* _model = nullptr;
    TelemetryPublisher* _telemetry = nullptr;
    SettingsStore* _settings = nullptr;

    ILogger* _logger = nullptr;
    FITLogger* _fitLogger = nullptr;
    TCXLogger* _tcxLogger = nullptr;
    CSVLogger* _csvLogger = nullptr;
    FtpEstimator _ftpEstimator;
    bool _loggerActive = false;

    float _rideElapsedSec = 0.0f;
    float _rideMovingSec = 0.0f;
    float _lastLapDistanceM = 0.0f;
    float _lastLapMovingSec = 0.0f;
    bool _lastSecondValid = false;
    uint8_t _lastSecond = 0;

    bool _autoPaused = false;
    uint32_t _stopSinceMs = 0;
    uint32_t _moveSinceMs = 0;

    void failLogger(const char* operation);

    static constexpr float AUTO_PAUSE_SPEED_KMH = 3.0f;
    static constexpr float AUTO_RESUME_SPEED_KMH = 5.0f;
    static constexpr float AUTO_RESUME_CADENCE_RPM = 20.0f;
    static constexpr uint32_t AUTO_PAUSE_DELAY_MS = 10000;
    static constexpr uint32_t AUTO_RESUME_DELAY_MS = 3000;
};

#endif /* RIDE_CONTROLLER_H */
