#ifndef FAULTREPORT_H
#define FAULTREPORT_H

// Captures the stacked registers of a CPU fault (HardFault/BusFault/UsageFault/
// MemManage) into RAM that survives a warm reset, forces a clean reset instead
// of hanging in the core's default `b .` handler, and prints the record at the
// next boot -- when a terminal is attached -- so field crashes are
// diagnosable with nothing more than a serial monitor.
//
// Device-only behaviour (NRF52840_XXAA); on the simulator this is a no-op so
// shared code (main.cpp) links on both targets.
void faultReportPrintIfPending();

// Initialise the fault-recorder: copy the vector table to RAM, patch the
// HardFault entry to route to our capture handler, and retarget SCB->VTOR.
// Call once from setup() before any fault can occur.
void faultReportInit();

#endif /* FAULTREPORT_H */