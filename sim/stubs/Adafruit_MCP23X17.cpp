#include "Adafruit_MCP23X17.h"
// Card present by default (SD_DET is active low).
static uint8_t g_gpioA = 0x00;
void    simSetMcpGPIOA(uint8_t v) { g_gpioA = v; }
uint8_t simGetMcpGPIOA() { return g_gpioA; }
