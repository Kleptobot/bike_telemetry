#include "SpiBusLock.hpp"
#if defined(ARDUINO_ARCH_NRF52)
SemaphoreHandle_t SpiBusLock::_mutex = nullptr;
#endif
