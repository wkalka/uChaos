#ifndef UCHAOS_SENSOR_H
#define UCHAOS_SENSOR_H

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/random/rand32.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "../uchaos_types.h"
#include "../uchaos_defs.h"
#include "../uchaos_console/uchaos_console.h"

#ifdef UCHAOS
#define sensor_channel_get(a, b, c)   uchaos_sensor_channel_get(a, b, c)
#endif

typedef void (*uchaos_sensor_data_func)(struct sensor_value* sensorValue);

void uchaos_sensorInit(const struct device* dev);

int uchaos_sensor_channel_get(const struct device* dev, enum sensor_channel chan, struct sensor_value* val);

bool uchaos_sensorConnection(void);
void uchaos_sensorNoise(struct sensor_value* sensorValue);
void uchaos_sensorDataAnomaly(struct sensor_value* sensorValue);
void uchaos_sensorDataSpike(struct sensor_value* value);
void uchaos_sensorOffset(struct sensor_value* value);
void uchaos_sensorStuckAtValue(struct sensor_value* value);

#endif