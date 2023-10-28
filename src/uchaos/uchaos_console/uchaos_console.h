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
#define CHAOS_CONSOLE_MSG_SIZE          32
#define CHAOS_CONSOLE_QUEUE_SIZE        5
#define CHAOS_CONSOLE_THREAD_STACKSIZE  512
#define CHAOS_CONSOLE_THREAD_PRIORITY   K_LOWEST_APPLICATION_THREAD_PRIO
#define CHAOS_CONSOLE_THREAD_SLEEP      2000
#define DIGITS_ASCII_START		        48
#define DIGITS_ASCII_END		        57

uChaos_SensorFault_t* chaos_consoleGetFaultsData(void);

void chaos_consoleInit(void);
void chaos_consoleThread(void* arg1, void* arg2, void* arg3);
void chaos_consoleThreadFunction(uint32_t sleep_ms, uint32_t id);

bool chaos_consoleSearchForCommand(uint8_t* buf);
bool chaos_consoleParseCommand(uint8_t* buf);
bool chaos_consoleSearchForSensorName(uint8_t* buf);
void chaos_consoleCheckCommand(uint8_t* buf);

void chaos_clearConsoleRxBuff(void);
uChaos_SensorFaultsTypes_t chaos_getFaultType(void);

#endif