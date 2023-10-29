#ifndef UCHAOS_SENSOR_H
#define UCHAOS_SENSOR_H

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/random/rand32.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../uchaos_types.h"
#include "../uchaos_defs.h"
#include "../uchaos_console/uchaos_console.h"

#ifdef UCHAOS
#define sensor_channel_get(a, b, c)   uChaosSensor_ChannelGet(a, b, c)
#endif


typedef struct 
{
    char name[UCHAOS_SENSOR_NAME_LEN];
    uChaos_SensorFault_t sensorFault;
    struct device* device;
} uChaosSensor_t;


typedef void (*uchaos_sensor_data_func)(struct sensor_value* sensorValue);

void uChaosSensor_Create(const char* name, const struct device* dev);
void uChaosSensor_Init(const char* name, const struct device* dev);

int uChaosSensor_ChannelGet(const struct device* dev, enum sensor_channel chan, struct sensor_value* val);

uChaosSensor_t* uChaosSensor_GetSensor(void);
void uChaosSensor_FaultSet(uChaosSensor_t* sensor, uChaos_SensorFault_t* fault);

bool uChaosSensor_Connection(void);
void uChaosSensor_Noise(struct sensor_value* sensorValue);
void uChaosSensor_DataAnomaly(struct sensor_value* sensorValue);
void uChaosSensor_DataSpike(struct sensor_value* value);
void uChaosSensor_Offset(struct sensor_value* value);
void uChaosSensor_StuckAtValue(struct sensor_value* value);

#endif