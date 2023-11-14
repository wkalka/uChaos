#include "uchaos_sensor.h"


extern int z_impl_sensor_channel_get(const struct device * dev, enum sensor_channel chan, struct sensor_value * val);

static uChaosSensor_t _uChaosSensors[UCHAOS_SENSORS_NUMBER];
static uint8_t _uChaosSensorsCount;

static uChaosSensor_DataFunc _uChaosSensor_DataFunctions[] = 
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

    if ( _uChaosSensorsCount < UCHAOS_MAX_SENSORS_NUMBER )
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
    if (_uChaosSensorsCount < UCHAOS_SENSORS_NUMBER)
    {
        snprintf(_uChaosSensors[_uChaosSensorsCount].name, UCHAOS_SENSOR_NAME_LEN, "%s", name);
        snprintf(_uChaosSensors[_uChaosSensorsCount].sensorFault.name, UCHAOS_FAULT_NAME_LEN, "%s", "none");
        _uChaosSensors[_uChaosSensorsCount].sensorFault.faultType = NONE;
        _uChaosSensors[_uChaosSensorsCount].sensorFault.paramsNbr = 0;
        _uChaosSensors[_uChaosSensorsCount].sensorFault.params = NULL;
        _uChaosSensors[_uChaosSensorsCount].device = (struct device*)dev;
        _uChaosSensorsCount++;
    }
}


int uChaosSensor_ChannelGet(const struct device* dev, enum sensor_channel chan, struct sensor_value* val)
{
    int retVal = z_impl_sensor_channel_get(dev, chan, val);
    uChaosSensor_t* sensor = uChaosSensor_GetSensor(dev);

    if (sensor->sensorFault.faultType != NONE)
    {
        if (sensor->sensorFault.faultType == CONNECTION)
        {
            if (uChaosSensor_Connection(sensor))
            {
                return -EIO;
            }
        }
        else
        {
            _uChaosSensor_DataFunctions[sensor->sensorFault.faultType - 2](val, sensor);
        }
    }
    sensor  = NULL;

    return retVal;
}


uChaosSensor_t* uChaosSensor_GetSensors(void)
{
    return _uChaosSensors;
}


uChaosSensor_t* uChaosSensor_GetSensor(const struct device* dev)
{
    for ( uint8_t i = 0; i <  UCHAOS_SENSORS_NUMBER; i++)
    {
        if (_uChaosSensors[i].device == dev)
        {
            return &_uChaosSensors[i];
        }
    }
    return NULL;
}


void uChaosSensor_SetFault(uChaosSensor_t* sensor, uChaos_Fault_t* fault)
{
    if ( sensor->sensorFault.params != NULL )
    {
        k_free(sensor->sensorFault.params);
        sensor->sensorFault.params = NULL;
    }
    memset(&sensor->sensorFault, 0, sizeof(uChaos_Fault_t));
    sensor->sensorFault.faultGroup = fault->faultGroup;
    sensor->sensorFault.faultType = fault->faultType;
    memset(sensor->sensorFault.name, 0, UCHAOS_FAULT_NAME_LEN);
    snprintf(sensor->sensorFault.name, UCHAOS_FAULT_NAME_LEN, "%s", (const char*)fault->name);
    sensor->sensorFault.params = (uint32_t*)k_calloc(fault->paramsNbr, sizeof(uint32_t));
    sensor->sensorFault.paramsNbr = fault->paramsNbr;
    for (uint8_t i = 0; i < sensor->sensorFault.paramsNbr; i++)
    {
        sensor->sensorFault.params[i] = fault->params[i];
        fault->params[i] = 0;
    }
}


bool uChaosSensor_Connection(uChaosSensor_t* sensor)
{
    static uint32_t eventCounter;
    static uint32_t faultFrequency;
    uint32_t minFaultFreq = sensor->sensorFault.params[0];
    uint32_t maxFaultFreq = sensor->sensorFault.params[1];

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


void uChaosSensor_Noise(struct sensor_value* value, uChaosSensor_t* sensor)
{
    double measurements[3] = {0};
    // double measurement = 0;

    uint32_t noiseMinPercent = sensor->sensorFault.params[0];
    uint32_t noiseMaxPercent = sensor->sensorFault.params[1];

    if (noiseMinPercent > noiseMaxPercent)
    {
        printk("Improper command params order\r\n");
        return;
    }

    uint32_t noisePercent = _uChaosSensor_RandUIntFromRange(noiseMinPercent, noiseMaxPercent);
    double noiseLevel = (double)(noisePercent / 100.0) + 1.0;

    // measurement = sensor_value_to_double(value) * noiseLevel;
    // sensor_value_from_double(value, measurement);

    for (uint8_t i = 0; i < (sizeof(measurements) / sizeof(measurements[0])); i++)
    {
        measurements[i] = sensor_value_to_double(&value[i]) * noiseLevel;
        sensor_value_from_double(&value[i], measurements[i]);    
    }
}


void uChaosSensor_DataAnomaly(struct sensor_value* value, uChaosSensor_t* sensor)
{
    static uint32_t eventCounter;
    static uint32_t faultFrequency;
    // double measurement = 0;
    double measurements[3] = {0};
    uint32_t minFaultFreq = sensor->sensorFault.params[0];
    uint32_t maxFaultFreq = sensor->sensorFault.params[1];
    uint32_t anomalyMinPercent = sensor->sensorFault.params[2];
    uint32_t anomalyMaxPercent = sensor->sensorFault.params[3];


    if ((minFaultFreq > maxFaultFreq) || (anomalyMinPercent > anomalyMaxPercent))
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

    for (uint8_t i = 0; i < (sizeof(measurements) / sizeof(measurements[0])); i++)
    {
        measurements[i] = sensor_value_to_double(&value[i]) * anomalyLevel;
        sensor_value_from_double(&value[i], measurements[i]);    
    }

    eventCounter = 0;
    faultFrequency = 0;
}


void uChaosSensor_DataSpike(struct sensor_value* value, uChaosSensor_t* sensor)
{

}


void uChaosSensor_Offset(struct sensor_value* value, uChaosSensor_t* sensor)
{

}


void uChaosSensor_StuckAtValue(struct sensor_value* value, uChaosSensor_t* sensor)
{

}
