#include "FaultReport.hpp"
#include <Arduino.h>

#if defined(NRF52840_XXAA)

#include <nrf.h>

namespace {

constexpr uint32_t FAULT_MAGIC = 0xF17B00C3u;

// Captured just before the forced reset. NOT in .bss/.data: the core's startup
// only initialises .data/.bss, and NVIC_SystemReset is a warm reset (RAM keeps
// its contents), so a dedicated .noinit section survives to the next boot,
// where faultReportPrintIfPending() reads it. The checksum defends against
// cold-boot garbage coincidentally matching the magic.
struct FaultRecord {
    uint32_t magic;
    uint32_t checksum;
    uint32_t pc, lr, xpsr;
    uint32_t frameSp;   // address of the stacked frame (R0..xPSR)
    uint32_t cfsr, hfsr;
    uint32_t heapFree;
};

volatile FaultRecord g_fault __attribute__((section(".noinit")));

// RAM-resident copy of the vector table, patched so HardFault routes to our
// capture handler instead of the core's silent-reset one. Placed in .noinit
// because faultReportInit() fills it at every boot; no need for the startup
// to zero it first. 512-byte alignment is required by SCB->VTOR (the NVIC
// needs the table aligned to its size, rounded up to >=128). 128 entries x 4
// B = 512 B covers all system-exception and IRQ slots on the nRF52840.
__attribute__((section(".noinit"), aligned(512)))
uint32_t g_vectorTable[128];

uint32_t checksumOf(const volatile FaultRecord& r) {
    return r.pc ^ r.lr ^ r.xpsr ^ r.frameSp ^ r.cfsr ^ r.hfsr ^ r.heapFree
           ^ 0x5A5A5A5Au;
}

}  // namespace

// Runs in fault context (priority -1): EVERY interrupt -- including the USBD
// IRQ that would transmit over CDC -- is masked here, so the Serial output
// below is best-effort only (it may sit in the FIFO and die with the reset);
// the boot-time print is the reliable path.
extern "C" [[noreturn]] void faultCapture(uint32_t* frame) {
    // frame -> {R0, R1, R2, R3, R12, LR, PC, xPSR} as pushed by exception entry.
    g_fault.pc      = frame[6];
    g_fault.lr      = frame[5];
    g_fault.xpsr    = frame[7];
    g_fault.frameSp = (uint32_t)(uintptr_t)frame;
    g_fault.cfsr    = SCB->CFSR;
    g_fault.hfsr    = SCB->HFSR;
    g_fault.heapFree = dbgHeapTotal() - dbgHeapUsed();
    g_fault.checksum = checksumOf(g_fault);
    g_fault.magic   = FAULT_MAGIC;   // set last: partial captures stay invalid

    Serial.printf("[FAULT] PC=0x%08x LR=0x%08x CFSR=0x%08x HFSR=0x%08x\n",
                  (unsigned)g_fault.pc, (unsigned)g_fault.lr,
                  (unsigned)g_fault.cfsr, (unsigned)g_fault.hfsr);

    NVIC_SystemReset();
    for (;;) { }
}

// The core's startup declares the peripheral-fault handlers WEAK (a `b .`
// hang), so a strong definition here overrides them. HardFault is different:
// utility/debug.cpp defines a STRONG HardFault_Handler that silently calls
// NVIC_SystemReset() -- the observed "MCU crashes and resets" with no output.
// Instead, faultReportInit() copies the vector table to RAM at boot and
// The trampoline picks the stack the faulting context was using (MSP for
// handler-mode faults, PSP for FreeRTOS tasks -- loop() runs on PSP) and
// tails into faultCapture, which never returns.
#define FAULT_TRAMPOLINE(name)                                   \
    extern "C" void name(void) __attribute__((naked));            \
    extern "C" void name(void) {                                  \
        __asm volatile("tst lr, #4        \n"                     \
                       "ite eq            \n"                     \
                       "mrseq r0, msp     \n"                     \
                       "mrsne r0, psp     \n"                     \
                       "b faultCapture    \n");                   \
    }

// Naked trampoline for the patched HardFault vector: picks MSP/PSP from
// EXC_RETURN and tails into faultCapture. Named myHardFaultHandler (not
// HardFault_Handler) to avoid the multiple-definition conflict with
// utility/debug.cpp's strong HardFault_Handler. The vector table entry is
// patched at runtime via VTOR (see faultReportInit) -- linker --wrap cannot
// intercept HardFault because it's already strongly defined by debug.cpp, and
// --wrap only redirects UNDEFINED references.
extern "C" void __attribute__((naked)) myHardFaultHandler(void) {
    __asm volatile(
        "tst lr, #4        \n"
        "ite eq            \n"
        "mrseq r0, msp     \n"
        "mrsne r0, psp     \n"
        "b faultCapture    \n"
    );
}

FAULT_TRAMPOLINE(BusFault_Handler)
FAULT_TRAMPOLINE(UsageFault_Handler)
FAULT_TRAMPOLINE(MemManage_Handler)

// Copy the vector table from flash (at the current SCB->VTOR) to the RAM copy
// above, patch the HardFault entry (slot 3) to route to myHardFaultHandler,
// and retarget SCB->VTOR to the RAM copy. Called once from setup() before any
// fault can occur. After this, all CPU faults are captured to .noinit and
// force a clean reset instead of the core's silent spin/reset.
void faultReportInit() {
    const uint32_t* src = reinterpret_cast<const uint32_t*>(SCB->VTOR);
    if (src == reinterpret_cast<const uint32_t*>(0)) {
        src = reinterpret_cast<const uint32_t*>(0x00000000u);
    }

    for (int i = 0; i < 128; ++i) {
        g_vectorTable[i] = src[i];
    }

    // Slot 3 = HardFault. Route it to our capture handler (bit 0 = Thumb).
    g_vectorTable[3] = reinterpret_cast<uint32_t>(myHardFaultHandler);

    SCB->VTOR = reinterpret_cast<uint32_t>(g_vectorTable);
}

void faultReportPrintIfPending() {
    // Copy the volatile record field-wise (an implicit struct copy cannot
    // bind a volatile source).
    FaultRecord r;
    r.magic = g_fault.magic;
    if (r.magic != FAULT_MAGIC) return;
    r.checksum = g_fault.checksum;
    r.pc       = g_fault.pc;
    r.lr       = g_fault.lr;
    r.xpsr     = g_fault.xpsr;
    r.frameSp  = g_fault.frameSp;
    r.cfsr     = g_fault.cfsr;
    r.hfsr     = g_fault.hfsr;
    r.heapFree = g_fault.heapFree;
    if (r.checksum != checksumOf(r)) return;

    // Do NOT consume the record until someone can actually see it: dropping it
    // without printing would throw away the only evidence the crash happened.
    // It persists across further resets until a terminal attaches.
    // Always print fault info; CDC check removed with TinyUSB

    g_fault.magic    = 0;   // consume before printing
    g_fault.checksum = 0;

    Serial.println("[FAULT] previous run died with a CPU fault:");
    Serial.printf("  PC=0x%08x LR=0x%08x xPSR=0x%08x frame@0x%08x\n",
                  (unsigned)r.pc, (unsigned)r.lr, (unsigned)r.xpsr,
                  (unsigned)r.frameSp);
    Serial.printf("  CFSR=0x%08x HFSR=0x%08x freeHeap=%u\n",
                  (unsigned)r.cfsr, (unsigned)r.hfsr, (unsigned)r.heapFree);
    Serial.println("  CFSR: low byte = MemManage flags, next = BusFault flags"
                   " (BFAR holds the address), high halfword = UsageFault flags.");
    Serial.println("  symbolicate PC: arm-none-eabi-addr2line -e .pio/build/"
                   "seeed-xiao-afruitnrf52-nrf52840-sense/firmware.elf -f -C -a <PC>");
    Serial.println("  (heap figure says how close to exhaustion the crash was)");
}

#else  // !NRF52840_XXAA -- simulator

void faultReportPrintIfPending() {}
void faultReportInit() {}

#endif