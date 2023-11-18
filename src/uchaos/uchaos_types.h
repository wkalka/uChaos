#ifndef UCHAOS_CONFIG_H
#define UCHAOS_CONFIG_H

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/rand32.h>

#include "uchaos_defs.h"

typedef enum
{
    SENSOR,
    MEMORY,
    CPU,
    POWER
    // NETWORK,
} uChaos_FaultGroup_t;

typedef enum 
{
    NONE,
    CONNECTION,
    NOISE,
    DATA_ANOMALY,
    DATA_SPIKE,
    OFFSET,
    STUCK_AT_VALUE,
    MEM_ALLOC,
    MEM_FREE,
    LOAD_ADD,
    LOAD_DEL,
    BATTERY,
    BATTERY_STOP,
    RESTART,
    HANG_UP
} uChaos_FaultType_t;

typedef struct
{
    char name[UCHAOS_FAULT_NAME_LEN];
    uChaos_FaultGroup_t faultGroup;
    uChaos_FaultType_t faultType;
    uint8_t paramsNbr;
    uint32_t* params;
} uChaos_Fault_t;

#endif