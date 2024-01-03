#ifndef UCHAOS_SENSOR_H
#define UCHAOS_SENSOR_H

#include "../uchaos_types.h"
#include "../uchaos_config.h"
#include "../uchaos_console/uchaos_console.h"

#if UCHAOS
#define sensor_channel_get(a, b, c)   uChaosSensor_ChannelGet(a, b, c)
#endif


typedef struct 
{
    char name[UCHAOS_SENSOR_NAME_LEN];
    uChaos_Fault_t sensorFault;
    struct device* device;
    double stuckValue[3];
    bool stuckAtValue;
} uChaosSensor_t;


typedef int (*uChaosSensor_DataFunc)(struct sensor_value* sensorValue, enum sensor_channel chan, uChaosSensor_t* sensor);

bool uChaosSensor_Create(const char* name, const struct device* dev);

int uChaosSensor_ChannelGet(const struct device* dev, enum sensor_channel chan, struct sensor_value* val);

uChaosSensor_t* uChaosSensor_GetSensors(void);
void uChaosSensor_SetCurrentSensor(uChaosSensor_t* sensor);
void uChaosSensor_SetFault(uChaos_Fault_t* fault);

#endif