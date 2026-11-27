#include "RideController.hpp"

#include <Arduino.h>

void RideController::begin(IStorage* storage, DataModel& model,
                           TelemetryPublisher& telemetry, SettingsStore& settings) {
    _storage = storage;
    _model = &model;
    _telemetry = &telemetry;
    _settings = &settings;
}

void RideController::configureLogger() {
    if (!_storage || !_model) return;

    const LoggerType selectedLogger = _model->bike().get().logger;
    delete _fitLogger;
    delete _tcxLogger;
    delete _csvLogger;
    _fitLogger = nullptr;
    _tcxLogger = nullptr;
    _csvLogger = nullptr;

    switch (selectedLogger) {
        case LoggerType::FIT:
            _fitLogger = new FITLogger(_storage);
            _logger = _fitLogger;
            Serial.println("FIT logger initialized.");
            break;
        case LoggerType::TCX:
            _tcxLogger = new TCXLogger(_storage, *_model);
            _logger = _tcxLogger;
            Serial.println("TCX logger initialized.");
            break;
        case LoggerType::CSV:
            _csvLogger = new CSVLogger(_storage, _model->bus());
            _logger = _csvLogger;
            Serial.println("CSV logger initialized.");
            break;
    }
}

void RideController::updateTimers(float dt, AppState state) {
    if (dt > 0.0f && dt < 5.0f) {
        if (state == AppState::LOGGING || state == AppState::PAUSED) {
            _rideElapsedSec += dt;
            if (state == AppState::LOGGING) _rideMovingSec += dt;
        }
    }
    _model->bus().publish(Topic::ElapsedSec, _rideElapsedSec);
    _model->bus().publish(Topic::MovingSec, _rideMovingSec);
}

void RideController::resetLapAnchors() {
    _lastLapDistanceM = 0.0f;
    _lastLapMovingSec = 0.0f;
}

void RideController::start(AppState& state) {
    state = AppState::LOGGING;
    _rideElapsedSec = 0.0f;
    _rideMovingSec = 0.0f;
    resetLapAnchors();
}

void RideController::stop(AppState& state) {
    state = AppState::IDLE;
}

void RideController::pause(AppState& state) {
    if (state == AppState::LOGGING && _logger) {
        _logger->pause(_model->time().get());
        state = AppState::PAUSED;
    }
}

void RideController::resume(AppState& state) {
    if (state == AppState::PAUSED) {
        if (_logger) _logger->resume(_model->time().get());
        state = AppState::LOGGING;
    }
}

void RideController::newLap(AppState state) {
    if (_logger && (state == AppState::LOGGING || state == AppState::PAUSED)) {
        _logger->newLap(_model->time().get());
        Serial.println("[RideController] New lap.");
    }
}

bool RideController::shouldAutoLap(AppState state) {
    if (!_model) return false;
    const BikeData& bike = _model->bike().get();
    if (bike.autoLapMode == AutoLapMode::Off || state != AppState::LOGGING) return false;

    if (bike.autoLapMode == AutoLapMode::Distance) {
        if (bike.autoLapValue == 0) return false;
        const float dist = _model->bus().get<float>(Topic::TotalDistanceM);
        if (dist - _lastLapDistanceM < (float)bike.autoLapValue * 1000.0f) return false;
    } else {
        if (bike.autoLapValue == 0) return false;
        if (_rideMovingSec - _lastLapMovingSec < (float)bike.autoLapValue * 60.0f) return false;
    }
    _lastLapDistanceM = _model->bus().get<float>(Topic::TotalDistanceM);
    _lastLapMovingSec = _rideMovingSec;
    return true;
}

AutoPauseAction RideController::updateAutoPause(AppState state, uint32_t now) {
    if (!_model || !_model->bike().get().autoPause) return AutoPauseAction::None;

    const float speedKmh = _model->bus().get<float>(Topic::EstimatedSpeed);
    const float cadence = _model->bus().get<float>(Topic::Cadence);
    const bool gpsValid = _model->bus().get<bool>(Topic::GpsValid);

    const bool speedUsable = gpsValid || speedKmh > 0.0f || cadence > 0.0f;
    if (!speedUsable) {
        _stopSinceMs = 0;
        _moveSinceMs = 0;
        return AutoPauseAction::None;
    }

    if (state == AppState::LOGGING && !_autoPaused) {
        if (speedKmh < AUTO_PAUSE_SPEED_KMH) {
            if (_stopSinceMs == 0) {
                _stopSinceMs = now;
            } else if (now - _stopSinceMs > AUTO_PAUSE_DELAY_MS) {
                _autoPaused = true;
                _stopSinceMs = 0;
                return AutoPauseAction::Pause;
            }
        } else {
            _stopSinceMs = 0;
        }
    } else if (state == AppState::PAUSED && _autoPaused) {
        if (speedKmh > AUTO_RESUME_SPEED_KMH || cadence > AUTO_RESUME_CADENCE_RPM) {
            if (_moveSinceMs == 0) {
                _moveSinceMs = now;
            } else if (now - _moveSinceMs > AUTO_RESUME_DELAY_MS) {
                _autoPaused = false;
                _moveSinceMs = 0;
                return AutoPauseAction::Resume;
            }
        } else {
            _moveSinceMs = 0;
        }
    } else {
        _stopSinceMs = 0;
        _moveSinceMs = 0;
        if (state == AppState::LOGGING) _autoPaused = false;
    }
    return AutoPauseAction::None;
}

void RideController::update(AppState state, AppState previousState) {
    if (!_model || !_telemetry || !_settings) return;
    const timeData& currentTime = _model->time().get();
    const DataBus& bus = _model->bus();
    const uint8_t second = currentTime.second();

    if (state == AppState::IDLE) {
        if ((previousState == AppState::LOGGING || previousState == AppState::PAUSED) && _logger) {
            Serial.println("[RideController] Finalizing logger.");
            _logger->finaliseLogging();

            if (_ftpEstimator.hasQualifyingWindow()) {
                uint16_t rideFtp = _ftpEstimator.suggestedFtp();
                if (rideFtp > _model->bio().get().ftpWatts) {
                    BioData updated = _model->bio().get();
                    updated.ftpWatts = rideFtp;
                    _model->bio().update(updated);
                    _settings->saveBiometrics();
                    Serial.print("[RideController] FTP PB updated to ");
                    Serial.println(rideFtp);
                }
            }
        }
        _telemetry->resetDistance();
        return;
    }

    if (state == AppState::LOGGING) {
        if (previousState != AppState::LOGGING && previousState != AppState::PAUSED) {
            if (_logger) _logger->startLogging(currentTime);
            _telemetry->resetDistance();
            _ftpEstimator.reset();
        }

        if (_logger) {
            _model->logger().update({_logger->elapsed_Total(), _logger->elapsed_Lap()});
        }

        if (!_lastSecondValid || second != _lastSecond) {
            if (_logger) {
                const location_data& loc = bus.get<location_data>(Topic::Location);
                Trackpoint tp;
                tp.latitude = loc.latitude;
                tp.longitude = loc.longitude;
                tp.altitude = bus.get<float>(Topic::FusedAltitude);
                tp.speed = bus.get<float>(Topic::EstimatedSpeed);
                tp.heartrate = bus.get<float>(Topic::HeartRate);
                tp.power = bus.get<float>(Topic::PowerMeter);
                tp.cadence = bus.get<float>(Topic::Cadence);
                tp.distance = bus.get<float>(Topic::TotalDistanceM);
                tp.calories = bus.get<float>(Topic::Calories);
                tp.normalizedPower = bus.get<float>(Topic::NormalizedPower);
                tp.intensityFactor = bus.get<float>(Topic::IntensityFactor);
                tp.tss = bus.get<float>(Topic::Tss);
                tp.hrZone = bus.get<uint8_t>(Topic::HrZone);
                tp.powerZone = bus.get<uint8_t>(Topic::PowerZone);
                _logger->addTrackpoint(tp, currentTime);

                const float powerWatts = bus.get<float>(Topic::PowerMeter) > 0.0f
                    ? bus.get<float>(Topic::PowerMeter)
                    : bus.get<float>(Topic::EstimatedPower);
                _ftpEstimator.addSample((uint16_t)(powerWatts > 0.0f ? powerWatts + 0.5f : 0.0f));
            }
        }
    }
    _lastSecondValid = true;
    _lastSecond = second;
}
