#include "uchaos_cpu.h"


K_THREAD_STACK_DEFINE(uChaosCPU_ThreadStack1, 512);
K_THREAD_STACK_DEFINE(uChaosCPU_ThreadStack2, 512);


// k_thread_stack_t * stacks [THREADS_NUMBER];
static struct k_thread _uChaosCPU_Thread[THREADS_NUMBER];
static k_tid_t _uChaosCPU_ThreadID[THREADS_NUMBER];
static uint8_t _uChaosCPU_ThreadPriority[THREADS_NUMBER] = 
{
    10,
    12
};
const char* _uChaosThread_Names[] = 
{
    "Thread1",
    "Thread2"
};


void uChaosCPU_Init(void)
{
    _uChaosCPU_ThreadID[0] = k_thread_create(&_uChaosCPU_Thread[0], uChaosCPU_ThreadStack1,
                                            K_THREAD_STACK_SIZEOF(uChaosCPU_ThreadStack1),
                                            uChaosCPU_Thread1,
                                            NULL, NULL, NULL,
                                            _uChaosCPU_ThreadPriority[0], 0, K_NO_WAIT);
    _uChaosCPU_ThreadID[1] = k_thread_create(&_uChaosCPU_Thread[1], uChaosCPU_ThreadStack2,
                                            K_THREAD_STACK_SIZEOF(uChaosCPU_ThreadStack2),
                                            uChaosCPU_Thread2,
                                            NULL, NULL, NULL,
                                            _uChaosCPU_ThreadPriority[1], 0, K_NO_WAIT);
    k_thread_name_set(_uChaosCPU_ThreadID[0], _uChaosThread_Names[0]);
    k_thread_name_set(_uChaosCPU_ThreadID[1], _uChaosThread_Names[1]);
    // for (uint8_t i = 0; i < THREADS_NUMBER; i++)
    // {
    //     stacks[i] = k_thread_stack_alloc(512, 0);
    //     _uChaosCPU_ThreadID[i] = k_thread_create(&_uChaosCPU_Thread[i], stacks[i],
    //                                             K_THREAD_STACK_SIZEOF(stacks[i]),
    //                                             uChaosCPU_Thread,
    //                                             NULL, NULL, NULL,
    //                                             K_LOWEST_THREAD_PRIO, 0, K_FOREVER);
    // }
}


void uChaosCPU_Thread1(void* arg1, void* arg2, void* arg3)
{
    uChaosCPU_ThreadFunction(1, arg2, arg3);
}


void uChaosCPU_Thread2(void* arg1, void* arg2, void* arg3)
{
    uChaosCPU_ThreadFunction(2, arg2, arg3);
}


void uChaosCPU_ThreadFunction(uint32_t arg1, void* arg2, void* arg3)
{
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
	static uint32_t x = 0;

	while (1)
    {
        if ( x == UINT32_MAX)
        {
            x = 0;
        }
        x++;
        printk("Thread uChaosCPU %d\r\n", arg1);
        k_sleep(K_MSEC(2000));
	}
}


void uChaosCPU_LoadAdd(const char* threadName)
{
    char threadState[64] = { 0 };
    for (uint8_t i = 0; i < THREADS_NUMBER; i++)
    {
        if (strcmp(k_thread_name_get(&_uChaosCPU_Thread[i]), threadName) == 0)
        {
            k_thread_state_str(_uChaosCPU_ThreadID[i], threadState, sizeof(threadState)/sizeof(threadState[0]));
        }
    }

    if (strcmp(threadState, "pending") == 0)
    {
        k_thread_start(_uChaosCPU_ThreadID[0]);
    }
    else if (strcmp(threadState, "suspended") == 0)
    {
        k_thread_resume(_uChaosCPU_ThreadID[0]);
    }
}


void uChaosCPU_LoadDel(const char* threadName)
{
    // k_thread_suspend(_uChaosCPU_ThreadID[0]);
}
