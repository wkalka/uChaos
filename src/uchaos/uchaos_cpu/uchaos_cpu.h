#ifndef UCHAOS_CPU_H 
#define UCHAOS_CPU_H

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "../uchaos_types.h"
#include "../uchaos_defs.h"


void uChaosCPU_Init(void);

char** uChaosCPU_GetThreadsNames(void);
void uChaosCPU_SetCurrentThread(const char* threadName);
void uChaosCPU_SetFault(uChaos_Fault_t* fault);

#endif