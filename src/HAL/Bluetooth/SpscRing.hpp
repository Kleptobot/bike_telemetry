#ifndef SPSCRING_H
#define SPSCRING_H

#include <Arduino.h>

/**
 * @class SpscRing
 * @brief Fixed-capacity lock-free queue for handing data from one producer
 *        thread to one consumer thread.
 *
 * Used to move BLE events off the SoftDevice callback task and onto the main
 * loop. Bluefruit dispatches scan and notify callbacks from its own FreeRTOS
 * task, so anything those callbacks touch is shared mutable state -- and a
 * std::vector reallocating under an iterator held by the other task is a
 * use-after-free, not merely a stale read.
 *
 * Correctness argument, since "lock-free" deserves one:
 *
 *  - Exactly one producer (the BLE task, calling push) and exactly one
 *    consumer (the main loop, calling pop). This is not safe for multiple
 *    producers.
 *  - The producer only ever writes _tail; the consumer only ever writes
 *    _head. Neither index is read-modify-written by both sides, so no
 *    atomic RMW is required.
 *  - Index loads and stores are single-byte and naturally aligned, so they
 *    are atomic on Cortex-M.
 *  - The __DMB() barriers order the slot write against the index publish
 *    (and the slot read against the index release), which is what stops the
 *    other side observing an index that points at a half-written slot.
 *  - Storage is a fixed array, so push() performs no allocation. Calling
 *    malloc from a SoftDevice callback is its own hazard.
 *
 * Capacity is N-1 entries: one slot is left empty so that full and empty
 * are distinguishable without a separate count that both sides would write.
 *
 * push() returns false and drops the item when full. For discovery events
 * that is the right behaviour -- losing one advertisement is harmless, the
 * peripheral will advertise again -- but callers that cannot tolerate loss
 * should check the return value.
 */
template <typename T, uint8_t N>
class SpscRing {
    static_assert(N >= 2, "SpscRing needs at least 2 slots");

public:
    /** Producer side. Safe to call from a BLE callback. */
    bool push(const T& value) {
        const uint8_t tail = _tail;
        const uint8_t next = (uint8_t)((tail + 1) % N);
        if (next == _head) return false;   // full -- drop rather than block

        _buf[tail] = value;
        __DMB();                            // publish the slot before the index
        _tail = next;
        return true;
    }

    /** Consumer side. Only call from the main loop. */
    bool pop(T& out) {
        const uint8_t head = _head;
        if (head == _tail) return false;    // empty

        out = _buf[head];
        __DMB();                            // finish reading before releasing
        _head = (uint8_t)((head + 1) % N);
        return true;
    }

    bool empty() const { return _head == _tail; }

private:
    T _buf[N];
    volatile uint8_t _head = 0;   // written by consumer only
    volatile uint8_t _tail = 0;   // written by producer only
};

#endif /* SPSCRING_H */
