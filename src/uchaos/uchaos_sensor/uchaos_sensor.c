#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/random/rand32.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "uchaos_sensor.h"


extern int z_impl_sensor_channel_get(const struct device * dev, enum sensor_channel chan, struct sensor_value * val);

static uChaosSensor_t _uChaosSensors[UCHAOS_SENSORS_NUMBER];
static uint8_t _uChaosSensorsCount;
static uChaosSensor_t* _currentSensor;

static int _uChaosSensor_Connection(struct sensor_value* value, enum sensor_channel chan, uChaosSensor_t* sensor);
static int _uChaosSensor_Noise(struct sensor_value* value, enum sensor_channel chan, uChaosSensor_t* sensor);
static int _uChaosSensor_DataAnomaly(struct sensor_value* value, enum sensor_channel chan, uChaosSensor_t* sensor);
static int _uChaosSensor_DataSpike(struct sensor_value* value, enum sensor_channel chan, uChaosSensor_t* sensor);
static int _uChaosSensor_Offset(struct sensor_value* value, enum sensor_channel chan, uChaosSensor_t* sensor);
static int _uChaosSensor_StuckAtValue(struct sensor_value* value, enum sensor_channel chan, uChaosSensor_t* sensor);

static uChaosSensor_DataFunc _uChaosSensor_DataFunctions[] = 
{
    _uChaosSensor_Connection,
    _uChaosSensor_Noise,
    _uChaosSensor_DataAnomaly,
    _uChaosSensor_DataSpike,
    _uChaosSensor_Offset,
    _uChaosSensor_StuckAtValue
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


static void _uChaosSensor_Init(const char* name, const struct device* dev)
{
    snprintf(_uChaosSensors[_uChaosSensorsCount].name, UCHAOS_SENSOR_NAME_LEN, "%s", name);
    snprintf(_uChaosSensors[_uChaosSensorsCount].sensorFault.name, UCHAOS_FAULT_NAME_LEN, "%s", "none");
    _uChaosSensors[_uChaosSensorsCount].sensorFault.faultType = NONE;
    _uChaosSensors[_uChaosSensorsCount].sensorFault.paramsNbr = 0;
    _uChaosSensors[_uChaosSensorsCount].sensorFault.params = NULL;
    _uChaosSensors[_uChaosSensorsCount].device = (struct device*)dev;
    memset(_uChaosSensors[_uChaosSensorsCount].stuckValue, 0.0, (sizeof(_uChaosSensors[_uChaosSensorsCount].stuckValue) / sizeof(_uChaosSensors[_uChaosSensorsCount].stuckValue[0])));
    _uChaosSensors[_uChaosSensorsCount].stuckAtValue = false;
    _uChaosSensorsCount++;
}


bool uChaosSensor_Create(const char* name, const struct device* dev)
{
    if ( name == NULL )
    {
        printk("ERROR: Sensor name not find\r\n");
        return false;
    }
    if ( dev == NULL )
    {
        printk("ERROR: Sensor device binding problem\r\n");
        return false;
    }

    if ( _uChaosSensorsCount < UCHAOS_SENSORS_NUMBER )
    {
        _uChaosSensor_Init(name, dev);
        return true;
    }
    else
    {
        printk("ERROR: Max number of sensors created\r\n");
        return false;
    }
}


static uChaosSensor_t* _uChaosSensor_GetSensor(const struct device* dev)
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


int uChaosSensor_ChannelGet(const struct device* dev, enum sensor_channel chan, struct sensor_value* val)
{
    int retVal = z_impl_sensor_channel_get(dev, chan, val);
    uChaosSensor_t* sensor = _uChaosSensor_GetSensor(dev);

    if (sensor->sensorFault.faultType != NONE)
    {
        if (retVal == 0)
        {
            retVal = _uChaosSensor_DataFunctions[sensor->sensorFault.faultType - 1](val, chan, sensor);
        }
    }
    else
    {
        if (sensor->stuckAtValue) { memset(sensor->stuckValue, 0.0, (sizeof(sensor->stuckValue) / sizeof(sensor->stuckValue[0]))); }
    }
    sensor  = NULL;

    return retVal;
}


uChaosSensor_t* uChaosSensor_GetSensors(void)
{
    return _uChaosSensors;
}


void uChaosSensor_SetCurrentSensor(uChaosSensor_t* sensor)
{
    _currentSensor = sensor;
}


void uChaosSensor_SetFault(uChaos_Fault_t* fault)
{
    if ( _currentSensor->sensorFault.params != NULL )
    {
        k_free(_currentSensor->sensorFault.params);
        _currentSensor->sensorFault.params = NULL;
    }
    memset(&_currentSensor->sensorFault, 0, sizeof(uChaos_Fault_t));
    _currentSensor->sensorFault.faultGroup = fault->faultGroup;
    _currentSensor->sensorFault.faultType = fault->faultType;
    memset(_currentSensor->sensorFault.name, 0, UCHAOS_FAULT_NAME_LEN);
    snprintf(_currentSensor->sensorFault.name, UCHAOS_FAULT_NAME_LEN, "%s", (const char*)fault->name);
    _currentSensor->sensorFault.params = (uint32_t*)k_calloc(fault->paramsNbr, sizeof(uint32_t));
    _currentSensor->sensorFault.paramsNbr = fault->paramsNbr;
    for (uint8_t i = 0; i < _currentSensor->sensorFault.paramsNbr; i++)
    {
        _currentSensor->sensorFault.params[i] = fault->params[i];
        fault->params[i] = 0;
    }
}


static int _uChaosSensor_Connection(struct sensor_value* value, enum sensor_channel chan, uChaosSensor_t* sensor)
{
    ARG_UNUSED(value);
    ARG_UNUSED(chan);

    int retVal = UCHAOS_EOK;
    static uint32_t eventCounter;
    static uint32_t faultFrequency;
    uint32_t minFaultFreq = sensor->sensorFault.params[0];
    uint32_t maxFaultFreq = sensor->sensorFault.params[1];

    if (minFaultFreq > maxFaultFreq)
    {
        printk("ERROR: Improper command params order\r\n");
        retVal = -UCHAOS_EINVAL;
        return retVal;
    }

    if (faultFrequency == 0)
    {
        faultFrequency = _uChaosSensor_RandUIntFromRange(minFaultFreq, maxFaultFreq);
    }

    while (eventCounter < faultFrequency)
    {
        eventCounter++;
        return retVal;
    }
    
    eventCounter = 0;
    faultFrequency = 0;
    retVal = -UCHAOS_EIO;

    return retVal;
}


static int _uChaosSensor_NoiseSingleChannel(struct sensor_value* value, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;
    double measurement = 0;

    uint32_t noiseMinPercent = sensor->sensorFault.params[0];
    uint32_t noiseMaxPercent = sensor->sensorFault.params[1];

    if (noiseMinPercent > noiseMaxPercent)
    {
        printk("ERROR: Improper command params order\r\n");
        retVal = -UCHAOS_EINVAL;
        return retVal;
    }

    uint32_t noisePercent = _uChaosSensor_RandUIntFromRange(noiseMinPercent, noiseMaxPercent);
    double noiseLevel = (double)(noisePercent / 100.0) + 1.0;

    measurement = sensor_value_to_double(value) * noiseLevel;
    sensor_value_from_double(value, measurement);

    return retVal;
}


static int _uChaosSensor_NoiseMultiChannel(struct sensor_value* value, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;
    double measurements[3] = {0};

    uint32_t noiseMinPercent = sensor->sensorFault.params[0];
    uint32_t noiseMaxPercent = sensor->sensorFault.params[1];

    if (noiseMinPercent > noiseMaxPercent)
    {
        printk("ERROR: Improper command params order\r\n");
        return retVal = -UCHAOS_EINVAL;
    }

    uint32_t noisePercent = _uChaosSensor_RandUIntFromRange(noiseMinPercent, noiseMaxPercent);
    double noiseLevel = (double)(noisePercent / 100.0) + 1.0;

    for (uint8_t i = 0; i < (sizeof(measurements) / sizeof(measurements[0])); i++)
    {
        measurements[i] = sensor_value_to_double(&value[i]) * noiseLevel;
        sensor_value_from_double(&value[i], measurements[i]);    
    }

    return retVal;
}


static int _uChaosSensor_Noise(struct sensor_value* value, enum sensor_channel chan, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;

    switch(chan)
    {
        case SENSOR_CHAN_ACCEL_XYZ:
        case SENSOR_CHAN_GYRO_XYZ:
        case SENSOR_CHAN_MAGN_XYZ:
            retVal = _uChaosSensor_NoiseMultiChannel(value, sensor);
            break;

        case SENSOR_CHAN_ACCEL_X:
        case SENSOR_CHAN_ACCEL_Y:
        case SENSOR_CHAN_ACCEL_Z:
        case SENSOR_CHAN_GYRO_X:
        case SENSOR_CHAN_GYRO_Y:
        case SENSOR_CHAN_GYRO_Z:
        case SENSOR_CHAN_MAGN_X:
        case SENSOR_CHAN_MAGN_Y:
        case SENSOR_CHAN_MAGN_Z:
        case SENSOR_CHAN_DIE_TEMP:
        case SENSOR_CHAN_AMBIENT_TEMP:
            retVal = _uChaosSensor_NoiseSingleChannel(value, sensor);
            break;

        default:
            printk("ERROR: Sensor type not handled\r\n");
            retVal = -UCHAOS_EINVAL;
            break;
    }

    return retVal;
}


static int _uChaosSensor_DataAnomalySingleChannel(struct sensor_value* value, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;
    static uint32_t eventCounter;
    static uint32_t faultFrequency;
    double measurement = 0;

    uint32_t minFaultFreq = sensor->sensorFault.params[0];
    uint32_t maxFaultFreq = sensor->sensorFault.params[1];
    uint32_t anomalyMinPercent = sensor->sensorFault.params[2];
    uint32_t anomalyMaxPercent = sensor->sensorFault.params[3];

    if ((minFaultFreq > maxFaultFreq) || (anomalyMinPercent > anomalyMaxPercent))
    {
        printk("ERROR: Improper command params order\r\n");
        retVal = -UCHAOS_EINVAL;
        return retVal;
    }

    if (faultFrequency == 0)
    {
        faultFrequency = _uChaosSensor_RandUIntFromRange(minFaultFreq, maxFaultFreq);
    }

    while (eventCounter < faultFrequency)
    {
        eventCounter++;
        return retVal;
    }
    
    uint32_t anomalyPercent = _uChaosSensor_RandUIntFromRange(anomalyMinPercent, anomalyMaxPercent);
    double anomalyLevel = (double)(anomalyPercent / 100.0) + 1.0;

    measurement = sensor_value_to_double(value) * anomalyLevel;
    sensor_value_from_double(value, measurement);

    eventCounter = 0;
    faultFrequency = 0;

    return retVal;
}


static int _uChaosSensor_DataAnomalyMultiChannel(struct sensor_value* value, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;
    static uint32_t eventCounter;
    static uint32_t faultFrequency;
    double measurements[3] = {0};

    uint32_t minFaultFreq = sensor->sensorFault.params[0];
    uint32_t maxFaultFreq = sensor->sensorFault.params[1];
    uint32_t anomalyMinPercent = sensor->sensorFault.params[2];
    uint32_t anomalyMaxPercent = sensor->sensorFault.params[3];


    if ((minFaultFreq > maxFaultFreq) || (anomalyMinPercent > anomalyMaxPercent))
    {
        printk("ERROR: Improper command params order\r\n");
        retVal = -UCHAOS_EINVAL;
        return retVal;
    }

    if (faultFrequency == 0)
    {
        faultFrequency = _uChaosSensor_RandUIntFromRange(minFaultFreq, maxFaultFreq);
    }

    while (eventCounter < faultFrequency)
    {
        eventCounter++;
        return retVal;
    }
    
    uint32_t anomalyPercent = _uChaosSensor_RandUIntFromRange(anomalyMinPercent, anomalyMaxPercent);
    double anomalyLevel = (double)(anomalyPercent / 100.0) + 1.0;

    for (uint8_t i = 0; i < (sizeof(measurements) / sizeof(measurements[0])); i++)
    {
        measurements[i] = sensor_value_to_double(&value[i]) * anomalyLevel;
        sensor_value_from_double(&value[i], measurements[i]);    
    }

    eventCounter = 0;
    faultFrequency = 0;
    
    return retVal;
}


static int _uChaosSensor_DataAnomaly(struct sensor_value* value, enum sensor_channel chan, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;

    switch(chan)
    {
        case SENSOR_CHAN_ACCEL_X:
        case SENSOR_CHAN_ACCEL_Y:
        case SENSOR_CHAN_ACCEL_Z:
        case SENSOR_CHAN_GYRO_X:
        case SENSOR_CHAN_GYRO_Y:
        case SENSOR_CHAN_GYRO_Z:
        case SENSOR_CHAN_MAGN_X:
        case SENSOR_CHAN_MAGN_Y:
        case SENSOR_CHAN_MAGN_Z:
        case SENSOR_CHAN_DIE_TEMP:
        case SENSOR_CHAN_AMBIENT_TEMP:
            retVal = _uChaosSensor_DataAnomalySingleChannel(value, sensor);
            break;

        case SENSOR_CHAN_ACCEL_XYZ:
        case SENSOR_CHAN_GYRO_XYZ:
        case SENSOR_CHAN_MAGN_XYZ:
            retVal = _uChaosSensor_DataAnomalyMultiChannel(value, sensor);
            break;

        default:
            printk("ERROR: Sensor type not handled\r\n");
            retVal = -UCHAOS_EINVAL;
            break;
    }

    return retVal;
}


static int _uChaosSensor_DataSpikeSingleChannel(struct sensor_value* value, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;

    return retVal;
}


static int _uChaosSensor_DataSpikeMultiChannel(struct sensor_value* value, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;

    return retVal;
}


static int _uChaosSensor_DataSpike(struct sensor_value* value, enum sensor_channel chan, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;

    switch(chan)
    {
        case SENSOR_CHAN_ACCEL_X:
        case SENSOR_CHAN_ACCEL_Y:
        case SENSOR_CHAN_ACCEL_Z:
        case SENSOR_CHAN_GYRO_X:
        case SENSOR_CHAN_GYRO_Y:
        case SENSOR_CHAN_GYRO_Z:
        case SENSOR_CHAN_MAGN_X:
        case SENSOR_CHAN_MAGN_Y:
        case SENSOR_CHAN_MAGN_Z:
        case SENSOR_CHAN_DIE_TEMP:
        case SENSOR_CHAN_AMBIENT_TEMP:
            retVal = _uChaosSensor_DataSpikeSingleChannel(value, sensor);
            break;

        case SENSOR_CHAN_ACCEL_XYZ:
        case SENSOR_CHAN_GYRO_XYZ:
        case SENSOR_CHAN_MAGN_XYZ:
            retVal = _uChaosSensor_DataSpikeMultiChannel(value, sensor);
            break;

        default:
            printk("ERROR: Sensor type not handled\r\n");
            retVal = -UCHAOS_EINVAL;
            break;
    }

    return retVal;
}


static int _uChaosSensor_OffsetSingleChannel(struct sensor_value* value, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;
    double measurement = 0;

    uint32_t offsetDirection = sensor->sensorFault.params[0];
    uint32_t offsetPercent = sensor->sensorFault.params[1];

    if (offsetPercent <= 0)
    {
        printk("ERROR: Improper command params order\r\n");
        retVal = -UCHAOS_EINVAL;
        return retVal;
    }

    int8_t direction = (offsetDirection == 0) ? 1 : -1;
    double offsetLevel = (double)(offsetPercent / 100.0);

    measurement = sensor_value_to_double(value);
    double offset  = measurement * offsetLevel;
    measurement = measurement + (direction * offset);
    sensor_value_from_double(value, measurement); 

    return retVal;
}


static int _uChaosSensor_OffsetMultiChannel(struct sensor_value* value, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;
    double measurements[3] = {0};

    uint32_t offsetDirection = sensor->sensorFault.params[0];
    uint32_t offsetPercent = sensor->sensorFault.params[1];

    if (offsetPercent <= 0)
    {
        printk("ERROR: Improper command params order\r\n");
        retVal = -UCHAOS_EINVAL;
        return retVal;
    }

    int8_t direction = (offsetDirection == 0) ? 1 : -1;
    double offsetLevel = (double)(offsetPercent / 100.0);

    for (uint8_t i = 0; i < (sizeof(measurements) / sizeof(measurements[0])); i++)
    {
        measurements[i] = sensor_value_to_double(&value[i]);
        double offset  = measurements[i] * offsetLevel;
        measurements[i] = measurements[i] + (direction * offset);
        sensor_value_from_double(&value[i], measurements[i]);    
    }

    return retVal;
}


static int _uChaosSensor_Offset(struct sensor_value* value, enum sensor_channel chan, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;

    switch(chan)
    {
        case SENSOR_CHAN_ACCEL_X:
        case SENSOR_CHAN_ACCEL_Y:
        case SENSOR_CHAN_ACCEL_Z:
        case SENSOR_CHAN_GYRO_X:
        case SENSOR_CHAN_GYRO_Y:
        case SENSOR_CHAN_GYRO_Z:
        case SENSOR_CHAN_MAGN_X:
        case SENSOR_CHAN_MAGN_Y:
        case SENSOR_CHAN_MAGN_Z:
        case SENSOR_CHAN_DIE_TEMP:
        case SENSOR_CHAN_AMBIENT_TEMP:
            retVal = _uChaosSensor_OffsetSingleChannel(value, sensor);
            break;

        case SENSOR_CHAN_ACCEL_XYZ:
        case SENSOR_CHAN_GYRO_XYZ:
        case SENSOR_CHAN_MAGN_XYZ:
            retVal = _uChaosSensor_OffsetMultiChannel(value, sensor);
            break;

        default:
            printk("ERROR: Sensor type not handled\r\n");
            retVal = -UCHAOS_EINVAL;
            break;
    }

    return retVal;
}


static void _uChaosSensor_StuckAtValueSingleChannel(struct sensor_value* value, uChaosSensor_t* sensor)
{
    if (sensor->stuckAtValue == false)
    { 
        sensor->stuckAtValue = true;
        sensor->stuckValue[0] = sensor_value_to_double(value);
    }

    sensor_value_from_double(value, sensor->stuckValue[0]);
}


static void _uChaosSensor_StuckAtValueMultiChannel(struct sensor_value* value, uChaosSensor_t* sensor)
{
    if (sensor->stuckAtValue == false)
    { 
        sensor->stuckAtValue = true;
        for (uint8_t i = 0; i < (sizeof(sensor->stuckValue) / sizeof(sensor->stuckValue[0])); i++)
        {
            sensor->stuckValue[i] = sensor_value_to_double(&value[i]);
        }
    }

    for (uint8_t i = 0; i < (sizeof(sensor->stuckValue) / sizeof(sensor->stuckValue[0])); i++)
    {
        sensor_value_from_double(&value[i], sensor->stuckValue[i]);
    }
}


static int _uChaosSensor_StuckAtValue(struct sensor_value* value, enum sensor_channel chan, uChaosSensor_t* sensor)
{
    int retVal = UCHAOS_EOK;

    switch(chan)
    {
        case SENSOR_CHAN_ACCEL_X:
        case SENSOR_CHAN_ACCEL_Y:
        case SENSOR_CHAN_ACCEL_Z:
        case SENSOR_CHAN_GYRO_X:
        case SENSOR_CHAN_GYRO_Y:
        case SENSOR_CHAN_GYRO_Z:
        case SENSOR_CHAN_MAGN_X:
        case SENSOR_CHAN_MAGN_Y:
        case SENSOR_CHAN_MAGN_Z:
        case SENSOR_CHAN_DIE_TEMP:
        case SENSOR_CHAN_AMBIENT_TEMP:
            _uChaosSensor_StuckAtValueSingleChannel(value, sensor);
            break;

        case SENSOR_CHAN_ACCEL_XYZ:
        case SENSOR_CHAN_GYRO_XYZ:
        case SENSOR_CHAN_MAGN_XYZ:
            _uChaosSensor_StuckAtValueMultiChannel(value, sensor);
            break;

        default:
            printk("ERROR: Sensor type not handled\r\n");
            retVal = -UCHAOS_EINVAL;
            break;
    }

    return retVal;
}
