#ifndef UCHAOS_CPU_H 
#define UCHAOS_CPU_H

#include "../uchaos_types.h"
#include "../uchaos_config.h"


void uChaosCPU_Init(void);

char** uChaosCPU_GetThreadsNames(void);
void uChaosCPU_SetCurrentThread(const char* threadName);
void uChaosCPU_SetFault(uChaos_Fault_t* fault);

#endif