#include "uchaos_sensor.h"


extern int z_impl_sensor_channel_get(const struct device * dev, enum sensor_channel chan, struct sensor_value * val);

static uChaosSensor_t _uChaosSensor[UCHAOS_SENSORS_NUMBER];
static uint8_t _uChaosSensorCount;

static uchaos_sensor_data_func _sensor_data_functions[STUCK_AT_VALUE - 1] = 
{
    uChaosSensor_Noise,
    uChaosSensor_DataAnomaly,
    uChaosSensor_DataSpike,
    uChaosSensor_Offset,
    uChaosSensor_StuckAtValue
};


static uint32_t _uChaosSensor_RandUIntFromRange(uint32_t low, uint32_t up)
{
    uint32_t random = sys_rand32_get();

    return ((random % (up - low + 1)) + low);
}

// static double _uChaosSensor_RandDoubleFromRange(double low, double up)
// {
//     double random = (double)sys_rand32_get();

//     return (random / (double)(UINT32_MAX / (up - low)) + low);
// }


void uChaosSensor_Create(const char* name, const struct device* dev)
{
    if ( name == NULL )
    {
        printk("ERROR: Sensor name not find\n");
        return;
    }
    if ( dev == NULL )
    {
        printk("ERROR: Sensor device binding problem\n");
        return;
    }

    if ( _uChaosSensorCount < UCHAOS_MAX_SENSORS_NUMBER )
    {
        uChaosSensor_Init(name, dev);
    }
    else
    {
        printk("ERROR: Max number of sensors created\n");
        return;
    }
}


void uChaosSensor_Init(const char* name, const struct device* dev)
{
    snprintf(_uChaosSensor[_uChaosSensorCount].name, UCHAOS_SENSOR_NAME_LEN - 1, "%s", name);
    snprintf(_uChaosSensor[_uChaosSensorCount].sensorFault.name, UCHAOS_FAULT_NAME_LEN - 1, "%s", "none");
    _uChaosSensor[_uChaosSensorCount].sensorFault.faultType = NONE;
    _uChaosSensor[_uChaosSensorCount].sensorFault.paramsNbr = 0;
    _uChaosSensor[_uChaosSensorCount].sensorFault.params = NULL;
    _uChaosSensor[_uChaosSensorCount].device = (struct device*)dev;
    _uChaosSensorCount++;
}


int uChaosSensor_ChannelGet(const struct device* dev, enum sensor_channel chan, struct sensor_value* val)
{
    int retVal = z_impl_sensor_channel_get(dev, chan, val);
    uChaos_SensorFaultsTypes_t faultType = chaos_getFaultType();

    if (faultType != NONE)
    {
        if (faultType == CONNECTION)
        {
            if (uChaosSensor_Connection())
            {
                return -EIO;
            }
        }
        else
        {
            _sensor_data_functions[faultType - 2](val);
        }
    }

    return retVal;
}


uChaosSensor_t* uChaosSensor_GetSensor(void)
{
    return _uChaosSensor;
}


void uChaosSensor_FaultSet(uChaosSensor_t* sensor, uChaos_SensorFault_t* fault)
{
    if ( sensor->sensorFault.params != NULL )
    {
        k_free(sensor->sensorFault.params);
        sensor->sensorFault.params = NULL;
    }
    memset(&sensor->sensorFault, 0, sizeof(uChaos_SensorFault_t));
    sensor->sensorFault.params = (uint32_t*)k_calloc(fault->paramsNbr, sizeof(uint32_t));
    sensor->sensorFault.paramsNbr = fault->paramsNbr;
    for (uint8_t i = 0; i < sensor->sensorFault.paramsNbr; i++)
    {
        sensor->sensorFault.params[i] = fault->params[i];
        fault->params[i] = 0;
    }
}


bool uChaosSensor_Connection(void)
{
    static uint32_t eventCounter;
    static uint32_t faultFrequency;
    uint32_t minFaultFreq = (uChaosConsole_GetFaultsData() + CONNECTION)->params[0];
    uint32_t maxFaultFreq = (uChaosConsole_GetFaultsData() + CONNECTION)->params[1];

    if (minFaultFreq > maxFaultFreq)
    {
        printk("Improper command params order\r\n");
        return false;
    }

    if (faultFrequency == 0)
    {
        faultFrequency = _uChaosSensor_RandUIntFromRange(minFaultFreq, maxFaultFreq);
    }

    while (eventCounter < faultFrequency)
    {
        eventCounter++;
        return false;
    }
    
    eventCounter = 0;
    faultFrequency = 0;
    return true;
}


void uChaosSensor_Noise(struct sensor_value* value)
{
    double measurements[3] = {0};
    // double measurement = 0;

    uint32_t noiseMinPercent = (uChaosConsole_GetFaultsData() + NOISE)->params[0];
    uint32_t noiseMaxPercent = (uChaosConsole_GetFaultsData() + NOISE)->params[1];

    if (noiseMinPercent > noiseMaxPercent)
    {
        printk("Improper command params order\r\n");
        return;
    }

    uint32_t noisePercent = _uChaosSensor_RandUIntFromRange(noiseMinPercent, noiseMaxPercent);
    double noiseLevel = (double)(noisePercent / 100.0) + 1.0;

    // measurement = sensor_value_to_double(value) * noiseLevel;

    // sensor_value_from_double(value, measurement);

    measurements[0] = sensor_value_to_double(&value[0]) * noiseLevel;
    measurements[1] = sensor_value_to_double(&value[1]) * noiseLevel;
    measurements[2] = sensor_value_to_double(&value[2]) * noiseLevel;

    sensor_value_from_double(&value[0], measurements[0]);
    sensor_value_from_double(&value[1], measurements[1]);
    sensor_value_from_double(&value[2], measurements[2]);
}


void uChaosSensor_DataAnomaly(struct sensor_value* value)
{
    static uint32_t eventCounter;
    static uint32_t faultFrequency;
    // double measurement = 0;
    double measurements[3] = {0};
    uint32_t minFaultFreq = (uChaosConsole_GetFaultsData() + DATA_ANOMALY)->params[0];
    uint32_t maxFaultFreq = (uChaosConsole_GetFaultsData() + DATA_ANOMALY)->params[1];
    uint32_t anomalyMinPercent = (uChaosConsole_GetFaultsData() + DATA_ANOMALY)->params[2];
    uint32_t anomalyMaxPercent = (uChaosConsole_GetFaultsData() + DATA_ANOMALY)->params[3];


    if (minFaultFreq > maxFaultFreq)
    {
        printk("Improper command params order\r\n");
        return;
    }
    if (anomalyMinPercent > anomalyMaxPercent)
    {
        printk("Improper command params order\r\n");
        return;
    }

    if (faultFrequency == 0)
    {
        faultFrequency = _uChaosSensor_RandUIntFromRange(minFaultFreq, maxFaultFreq);
    }

    while (eventCounter < faultFrequency)
    {
        eventCounter++;
        return;
    }
    
    uint32_t anomalyPercent = _uChaosSensor_RandUIntFromRange(anomalyMinPercent, anomalyMaxPercent);
    double anomalyLevel = (double)(anomalyPercent / 100.0) + 1.0;

    // measurement = sensor_value_to_double(value) * anomalyLevel;

    // sensor_value_from_double(value, measurement);

    measurements[0] = sensor_value_to_double(&value[0]) * anomalyLevel;
    measurements[1] = sensor_value_to_double(&value[1]) * anomalyLevel;
    measurements[2] = sensor_value_to_double(&value[2]) * anomalyLevel;

    sensor_value_from_double(&value[0], measurements[0]);
    sensor_value_from_double(&value[1], measurements[1]);
    sensor_value_from_double(&value[2], measurements[2]);

    eventCounter = 0;
    faultFrequency = 0;
}


void uChaosSensor_DataSpike(struct sensor_value* value)
{

}


void uChaosSensor_Offset(struct sensor_value* value)
{

}


void uChaosSensor_StuckAtValue(struct sensor_value* value)
{

}
