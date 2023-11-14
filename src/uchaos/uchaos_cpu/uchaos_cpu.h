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

#define THREADS_NUMBER      1


void uChaosCPU_Init(void);
void uChaosCPU_Thread(void* arg1, void* arg2, void* arg3);
void uChaosCPU_ThreadFunction(void* arg1, void* arg2, void* arg3);

void uChaosCPU_LoadAdd(void);
void uChaosCPU_LoadDel(void);

#endif