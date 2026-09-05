#include "Fusion/FtpEstimator.hpp"

FtpEstimator::FtpEstimator()
    : _idx(0), _count(0), _sum(0), _bestAvg(0) {
    memset(_buf, 0, sizeof(_buf));
}

void FtpEstimator::addSample(uint16_t watts) {
    // If the buffer is full, subtract the oldest sample from the running
    // sum before overwriting it -- keeps the average O(1) per sample.
    if (_count >= WINDOW_SECONDS) {
        _sum -= _buf[_idx];
    } else {
        ++_count;
    }

    _buf[_idx] = watts;
    _sum += watts;
    _idx = (_idx + 1) % WINDOW_SECONDS;

    // Only consider full windows for the best average.
    if (_count >= WINDOW_SECONDS) {
        uint16_t avg = (uint16_t)(_sum / WINDOW_SECONDS);
        if (avg > _bestAvg) {
            _bestAvg = avg;
        }
    }
}

uint16_t FtpEstimator::suggestedFtp() const {
    if (_bestAvg == 0) return 0;
    // x0.95 in integer math: multiply by 95 then divide by 100.
    return (uint16_t)(((uint32_t)_bestAvg * 95u) / 100u);
}

void FtpEstimator::reset() {
    _idx = 0;
    _count = 0;
    _sum = 0;
    _bestAvg = 0;
    memset(_buf, 0, sizeof(_buf));
}