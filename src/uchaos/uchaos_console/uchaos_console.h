#ifndef UCHAOS_CONSOLE_H 
#define UCHAOS_CONSOLE_H

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "../uchaos_types.h"
#include "../uchaos_defs.h"
#include "../uchaos_sensor/uchaos_sensor.h"


#define UART_DEV_NODE                   DT_NODELABEL(uart0)
#define DIGITS_ASCII_START		        48
#define DIGITS_ASCII_END		        57

uChaos_SensorFault_t* uChaosConsole_GetFaultsData(void);

void uChaosConsole_Init(void);
void uChaosConsole_Thread(void* arg1, void* arg2, void* arg3);
void uChaosConsole_ThreadFunction(uint32_t sleep_ms, uint32_t id);

bool uChaosConsole_SearchForFault(uint8_t* buf);
bool uChaosConsole_ParseCommand(uint8_t* buf);
bool uChaosConsole_SearchForSensorName(uint8_t* buf);
void uChaosConsole_CheckCommand(uint8_t* buf);

uChaos_SensorFaultsTypes_t chaos_getFaultType(void);

#endif