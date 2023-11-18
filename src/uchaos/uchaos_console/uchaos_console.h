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
#include "../uchaos_cpu/uchaos_cpu.h"
#include "../uchaos_battery/uchaos_battery.h"


#define UART_DEV_NODE                   DT_NODELABEL(uart0)
#define DIGITS_ASCII_START		        48
#define DIGITS_ASCII_END		        57


void uChaosConsole_Init(void);
void uChaosConsole_Thread(void* arg1, void* arg2, void* arg3);
void uChaosConsole_ThreadFunction(uint32_t sleep_ms, uint32_t id);

bool uChaosConsole_SearchForFault(uint8_t* buf);
bool uChaosConsole_SearchForStringParam(uint8_t* destination, uint8_t* source, uint8_t* index);
bool uChaosConsole_SearchForSensorName(uint8_t* buf);
bool uChaosConsole_SearchForThreadName(uint8_t* buf);
bool uChaosConsole_SetFault(uChaos_Fault_t* fault);
bool uChaosConsole_ParseCommand(uint8_t* buf, uint8_t* index);
void uChaosConsole_CheckCommand(uint8_t* buf);

void uChaosConsole_Help(void);

#endif