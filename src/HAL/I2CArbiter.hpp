#ifndef I2CARBITER_H
#define I2CARBITER_H

#include <Arduino.h>

/**
 * I2CArbiter -- the bus-sharing policy between the LC76G and every other
 * I2C device, stated in one place.
 *
 * The LC76G's I2C interface must not see foreign traffic mid-operation, and
 * its internal NMEA buffer backs up if its drain cycles get crowded out.
 * Three rules follow; all are observable in HAL's [i2c] summary:
 *
 *   1. EXCLUSION    -- while the GNSS state machine is mid-operation
 *                      (LC76G::isBusy), no foreign device touches the bus.
 *   2. SETTLE       -- after any foreign transaction the GNSS's next cycle
 *                      start is postponed (HAL calls LC76G::i2c_wait, 10 ms),
 *                      giving the module a quiet bus before it drives the
 *                      lines again.
 *   3. BACKPRESSURE -- the GNSS normally STARTS a bus cycle every ~50-60 ms
 *                      (23/s measured at stock rates). If it has not managed
 *                      to start one for MAX_CYCLE_START_GAP_MS, foreign
 *                      devices yield until it catches up. Gating on cycle
 *                      starts -- not drains -- means a quiet module
 *                      (searching, no fix) never starves the sensors: idle
 *                      probes count as turns, so only genuine crowding-out
 *                      trips this rule.
 *
 * Batching: a granted tick runs every due foreign device in one pass, so
 * the per-second grant count -- and with it the settle cost of rule 2 --
 * is bounded by the slowest device period, not by the device count.
 */
class I2CArbiter {
public:
    /**
     * Decide whether foreign devices (inputs, sensors) may take the bus.
     *
     * @param gnssBusy               LC76G::isBusy() -- a cycle is in progress
     * @param msSinceLastCycleStart  LC76G::msSinceLastCycleStart(); 0 = no
     *                               cycle yet this power cycle (booting),
     *                               which never trips backpressure
     * @return true when the bus is unavailable to foreign devices
     */
    bool foreignBusy(bool gnssBusy, uint32_t msSinceLastCycleStart) {
        if (gnssBusy) {
            return true;                                   // rule 1
        }
        if (msSinceLastCycleStart != 0 &&
            msSinceLastCycleStart > MAX_CYCLE_START_GAP_MS) {
            ++_backpressureTicks;                          // rule 3
            return true;
        }
        return false;
    }

    // Lifetime count of ticks where rule 3 held foreign devices off. Zero
    // in normal operation; nonzero means the GNSS was being crowded out and
    // the yield engaged (visible in the [i2c] summary as bp=).
    uint32_t dbgBackpressureTicks() const { return _backpressureTicks; }

    // A healthy state machine starts a cycle every ~50-60 ms; the worst
    // legitimate stretching (foreign pass + settle) stays well under 150 ms.
    // 300 ms is ~5x normal -- generous against false trips, and still catches
    // real crowding-out an order of magnitude before the module's buffer is
    // at risk at any measured production rate.
    static const uint32_t MAX_CYCLE_START_GAP_MS = 300;

private:
    uint32_t _backpressureTicks = 0;
};

#endif /* I2CARBITER_H */