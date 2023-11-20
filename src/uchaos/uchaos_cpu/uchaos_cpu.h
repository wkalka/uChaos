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

#define THREADS_NUMBER      2


void uChaosCPU_Init(void);
void uChaosCPU_Thread1(void* arg1, void* arg2, void* arg3);
void uChaosCPU_Thread2(void* arg1, void* arg2, void* arg3);
void uChaosCPU_ThreadFunction(uint32_t arg1, void* arg2, void* arg3);

void uChaosCPU_LoadAdd(const char* threadName);
void uChaosCPU_LoadDel(const char* threadName);

#endif