#include "uchaos_cpu.h"


K_THREAD_STACK_DEFINE(uChaosCPU_ThreadStack1, 512);
// K_THREAD_STACK_DEFINE(uChaosCPU_ThreadStack2, 256);


static struct k_thread _uChaosCPU_ThreadStruct[THREADS_NUMBER];

static k_tid_t _uChaosCPU_ThreadID[THREADS_NUMBER];

// static uint8_t _uChaosCPU_ThreadPriority[THREADS_NUMBER];

k_thread_stack_t * stacks [THREADS_NUMBER];


void uChaosCPU_Init(void)
{
    for (uint8_t i = 0; i < THREADS_NUMBER; i++)
    {
        // stacks[i] = k_thread_stack_alloc(512, 0);
        // _uChaosCPU_ThreadID[i] = k_thread_create(&_uChaosCPU_ThreadStruct[i], stacks[i],
        //                                         K_THREAD_STACK_SIZEOF(stacks[i]),
        //                                         uChaosCPU_Thread,
        //                                         NULL, NULL, NULL,
        //                                         K_LOWEST_THREAD_PRIO, 0, K_FOREVER);
        _uChaosCPU_ThreadID[i] = k_thread_create(&_uChaosCPU_ThreadStruct[i], uChaosCPU_ThreadStack1,
                                                K_THREAD_STACK_SIZEOF(uChaosCPU_ThreadStack1),
                                                uChaosCPU_Thread,
                                                NULL, NULL, NULL,
                                                K_LOWEST_THREAD_PRIO, 0, K_NO_WAIT);
    }
}


void uChaosCPU_Thread(void* arg1, void* arg2, void* arg3)
{
    uChaosCPU_ThreadFunction(arg1, arg2, arg3);
}


void uChaosCPU_ThreadFunction(void* arg1, void* arg2, void* arg3)
{
	static uint32_t x = 0;

	while (1) {
        if ( x == UINT32_MAX)
        {
            x = 0;
        }
        x++;
        printk("Thread uChaosCPU\r\n");
        k_sleep(K_MSEC(2000));
	}
}


void uChaosCPU_LoadAdd(void)
{
    /* check state if not started start */
    // k_thread_start(_uChaosCPU_ThreadID[0]);
    k_thread_resume(_uChaosCPU_ThreadID[0]);
}


void uChaosCPU_LoadDel(void)
{
    k_thread_suspend(_uChaosCPU_ThreadID[0]);
}
