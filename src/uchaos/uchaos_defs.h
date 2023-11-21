#ifndef UCHAOS_DEFS_H 
#define UCHAOS_DEFS_H

#define UCHAOS                           1

/* UCHAOS CONSOLE DEFS */
#define UCHAOS_CONSOLE_QUEUE_SIZE        5
#define UCHAOS_CONSOLE_QUEUE_ALIGN       4
#define UCHAOS_CONSOLE_MSG_SIZE          32
#define UCHAOS_CONSOLE_THREAD_STACKSIZE  512
#define UCHAOS_CONSOLE_THREAD_PRIORITY   K_LOWEST_APPLICATION_THREAD_PRIO
#define UCHAOS_CONSOLE_THREAD_SLEEP      2000

/* UCHAOS SENSOR DEFS */
#define UCHAOS_SENSORS_NUMBER            2
#define UCHAOS_SENSOR_NAME_LEN           24
#define UCHAOS_MAX_SENSORS_NUMBER        2

/* UCHAOS THREADS DEFS */
#define THREADS_NUMBER                   3

#define UCHAOS_FAULT_NAME_LEN            16

#endif