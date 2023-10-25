#ifndef UCHAOS_CONFIG_H
#define UCHAOS_CONFIG_H

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/rand32.h>

typedef enum
{
    HARDWARE,
    SENSOR,
    PROTOCOL,
    NETWORK,
} chaos_faultsGroups_t;

typedef enum 
{
    NONE,
    CONNECTION,
    NOISE,
    DATA_ANOMALY,
    DATA_SPIKE,
    OFFSET,
    STUCK_AT_VALUE
} chaos_sensorFaultsTypes_t;

typedef struct
{
    char* name;
    chaos_sensorFaultsTypes_t faultType;
    uint8_t paramsNbr;
    uint32_t* params;
}chaos_sensorFault_t;

#endif