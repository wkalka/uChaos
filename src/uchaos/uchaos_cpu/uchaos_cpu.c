#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/time_units.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "uchaos_cpu.h"


K_THREAD_STACK_DEFINE(uChaosCPU_ThreadStack1, 512);
K_THREAD_STACK_DEFINE(uChaosCPU_ThreadStack2, 512);
K_THREAD_STACK_DEFINE(uChaosCPU_ThreadStack3, 512);
K_THREAD_STACK_DEFINE(uChaosCPU_ThreadStack4, 512);
K_THREAD_STACK_DEFINE(uChaosCPU_ThreadStack5, 512);
K_THREAD_STACK_DEFINE(uChaosCPU_ThreadStack6, 512);
K_THREAD_STACK_DEFINE(uChaosCPU_ThreadStack7, 512);
K_THREAD_STACK_DEFINE(uChaosCPU_ThreadStack8, 512);

static struct k_thread _uChaosCPU_Thread[THREADS_NUMBER];
static k_tid_t _uChaosCPU_ThreadID[THREADS_NUMBER];
static uint8_t _uChaosCPU_ThreadPriority[THREADS_NUMBER] = 
{
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12
};
char* _uChaosThread_Names[] = 
{
    "Thread1",
    "Thread2",
    "Thread3",
    "Thread4",
    "Thread5",
    "Thread6",
    "Thread7",
    "Thread8"
};
static const char* _currentThreadName = NULL;

static __thread uint32_t loopCounter;
static __thread int64_t upTime;
static __thread int64_t nowTime;


static void uChaosCPU_ThreadFunction(const char* threadName, void* arg2, void* arg3)
{
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint32_t loopDelayTimeMs = 300;
    uint32_t threadSleepTimeMs = 1000;

    upTime = k_uptime_get();
    nowTime = k_uptime_get();
	while (1)
    {
        while((nowTime - upTime) < loopDelayTimeMs)
        {
            loopCounter++;
            if ( loopCounter == UINT32_MAX)
            {
                loopCounter = 0;
            }
            nowTime = k_uptime_get();
        }
        k_sleep(K_MSEC(threadSleepTimeMs));
        upTime = k_uptime_get();
        printk("Thread uChaosCPU: %s\r\n", threadName);
	}
}


static void uChaosCPU_Thread1(void* arg1, void* arg2, void* arg3)
{
    uChaosCPU_ThreadFunction(_uChaosThread_Names[0], arg2, arg3);
}


static void uChaosCPU_Thread2(void* arg1, void* arg2, void* arg3)
{
    uChaosCPU_ThreadFunction(_uChaosThread_Names[1], arg2, arg3);
}


static void uChaosCPU_Thread3(void* arg1, void* arg2, void* arg3)
{
    uChaosCPU_ThreadFunction(_uChaosThread_Names[2], arg2, arg3);
}


static void uChaosCPU_Thread4(void* arg1, void* arg2, void* arg3)
{
    uChaosCPU_ThreadFunction(_uChaosThread_Names[3], arg2, arg3);
}


static void uChaosCPU_Thread5(void* arg1, void* arg2, void* arg3)
{
    uChaosCPU_ThreadFunction(_uChaosThread_Names[4], arg2, arg3);
}


static void uChaosCPU_Thread6(void* arg1, void* arg2, void* arg3)
{
    uChaosCPU_ThreadFunction(_uChaosThread_Names[5], arg2, arg3);
}


static void uChaosCPU_Thread7(void* arg1, void* arg2, void* arg3)
{
    uChaosCPU_ThreadFunction(_uChaosThread_Names[6], arg2, arg3);
}


static void uChaosCPU_Thread8(void* arg1, void* arg2, void* arg3)
{
    uChaosCPU_ThreadFunction(_uChaosThread_Names[7], arg2, arg3);
}


static void uChaosCPU_LoadAdd(const char* threadName)
{
    char threadState[64] = { 0 };
    uint8_t i;
    for (i = 0; i < THREADS_NUMBER; i++)
    {
        if (strcmp(_uChaosThread_Names[i], threadName) == 0)
        {
            k_thread_state_str(_uChaosCPU_ThreadID[i], threadState, sizeof(threadState)/sizeof(threadState[0]));
            break;
        }
    }

    if (strcmp(threadState, "prestart") == 0)
    {
        k_thread_start(_uChaosCPU_ThreadID[i]);
    }
    else if (strcmp(threadState, "suspended") == 0)
    {
        k_thread_resume(_uChaosCPU_ThreadID[i]);
    }
}


static void uChaosCPU_LoadDel(const char* threadName)
{
    for (uint8_t i = 0; i < THREADS_NUMBER; i++)
    {
        if (strcmp(_uChaosThread_Names[i], threadName) == 0)
        {
            k_thread_suspend(_uChaosCPU_ThreadID[i]);
        }
    }
}


void uChaosCPU_Init(void)
{
    uint8_t i = 0;
    _uChaosCPU_ThreadID[i] = k_thread_create(&_uChaosCPU_Thread[i], uChaosCPU_ThreadStack1,
                                            K_THREAD_STACK_SIZEOF(uChaosCPU_ThreadStack1),
                                            uChaosCPU_Thread1,
                                            NULL, NULL, NULL,
                                            _uChaosCPU_ThreadPriority[i], 0, K_FOREVER);
    k_thread_name_set(_uChaosCPU_ThreadID[i], _uChaosThread_Names[i]);
    i++;
    _uChaosCPU_ThreadID[i] = k_thread_create(&_uChaosCPU_Thread[i], uChaosCPU_ThreadStack2,
                                            K_THREAD_STACK_SIZEOF(uChaosCPU_ThreadStack2),
                                            uChaosCPU_Thread2,
                                            NULL, NULL, NULL,
                                            _uChaosCPU_ThreadPriority[i], 0, K_FOREVER);
    k_thread_name_set(_uChaosCPU_ThreadID[i], _uChaosThread_Names[i]);
    i++;
    _uChaosCPU_ThreadID[i] = k_thread_create(&_uChaosCPU_Thread[i], uChaosCPU_ThreadStack3,
                                            K_THREAD_STACK_SIZEOF(uChaosCPU_ThreadStack3),
                                            uChaosCPU_Thread3,
                                            NULL, NULL, NULL,
                                            _uChaosCPU_ThreadPriority[i], 0, K_FOREVER);
    k_thread_name_set(_uChaosCPU_ThreadID[i], _uChaosThread_Names[i]);
    i++;
    _uChaosCPU_ThreadID[i] = k_thread_create(&_uChaosCPU_Thread[i], uChaosCPU_ThreadStack4,
                                            K_THREAD_STACK_SIZEOF(uChaosCPU_ThreadStack4),
                                            uChaosCPU_Thread4,
                                            NULL, NULL, NULL,
                                            _uChaosCPU_ThreadPriority[i], 0, K_FOREVER);
    k_thread_name_set(_uChaosCPU_ThreadID[i], _uChaosThread_Names[i]);
    i++;
    _uChaosCPU_ThreadID[i] = k_thread_create(&_uChaosCPU_Thread[i], uChaosCPU_ThreadStack5,
                                            K_THREAD_STACK_SIZEOF(uChaosCPU_ThreadStack5),
                                            uChaosCPU_Thread5,
                                            NULL, NULL, NULL,
                                            _uChaosCPU_ThreadPriority[i], 0, K_FOREVER);
    k_thread_name_set(_uChaosCPU_ThreadID[i], _uChaosThread_Names[i]);
    i++;
    _uChaosCPU_ThreadID[i] = k_thread_create(&_uChaosCPU_Thread[i], uChaosCPU_ThreadStack6,
                                            K_THREAD_STACK_SIZEOF(uChaosCPU_ThreadStack6),
                                            uChaosCPU_Thread6,
                                            NULL, NULL, NULL,
                                            _uChaosCPU_ThreadPriority[i], 0, K_FOREVER);
    k_thread_name_set(_uChaosCPU_ThreadID[i], _uChaosThread_Names[i]);
    i++;
    _uChaosCPU_ThreadID[i] = k_thread_create(&_uChaosCPU_Thread[i], uChaosCPU_ThreadStack7,
                                            K_THREAD_STACK_SIZEOF(uChaosCPU_ThreadStack7),
                                            uChaosCPU_Thread7,
                                            NULL, NULL, NULL,
                                            _uChaosCPU_ThreadPriority[i], 0, K_FOREVER);
    k_thread_name_set(_uChaosCPU_ThreadID[i], _uChaosThread_Names[i]);
    i++;
    _uChaosCPU_ThreadID[i] = k_thread_create(&_uChaosCPU_Thread[i], uChaosCPU_ThreadStack8,
                                            K_THREAD_STACK_SIZEOF(uChaosCPU_ThreadStack8),
                                            uChaosCPU_Thread8,
                                            NULL, NULL, NULL,
                                            _uChaosCPU_ThreadPriority[i], 0, K_FOREVER);
    k_thread_name_set(_uChaosCPU_ThreadID[i], _uChaosThread_Names[i]);
}


char** uChaosCPU_GetThreadsNames(void)
{
    return _uChaosThread_Names;
}


void uChaosCPU_SetCurrentThread(const char* threadName)
{
    _currentThreadName = threadName;
}


void uChaosCPU_SetFault(uChaos_Fault_t* fault)
{
    switch (fault->faultType)
    {
        case LOAD_ADD:
        {
            uChaosCPU_LoadAdd(_currentThreadName);
            break;
        }
        case LOAD_DEL:
        {
            uChaosCPU_LoadDel(_currentThreadName);
            break;
        }    
        default:
        {
            break;
        }
    }

    _currentThreadName = NULL;
}
