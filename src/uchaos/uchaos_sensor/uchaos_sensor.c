#include "uchaos_sensor.h"


extern int z_impl_sensor_channel_get(const struct device * dev, enum sensor_channel chan, struct sensor_value * val);

// todo: add stucked value
static struct device* _chaos_sensor;
static uchaos_sensor_data_func _sensor_data_functions[STUCK_AT_VALUE - 1] = 
{
    uchaos_sensorNoise,
    uchaos_sensorDataAnomaly,
    uchaos_sensorDataSpike,
    uchaos_sensorOffset,
    uchaos_sensorStuckAtValue
};


static uint32_t _chaos_generateUIntRandFromRange(uint32_t low, uint32_t up)
{
    uint32_t random = sys_rand32_get();

    return ((random % (up - low + 1)) + low);
}

// static double _chaos_generateDoubleRandFromRange(double low, double up)
// {
//     double random = (double)sys_rand32_get();

//     return (random / (double)(UINT32_MAX / (up - low)) + low);
// }


void uchaos_sensorInit(const struct device* dev)
{
    _chaos_sensor = (struct device*)dev;

    if (_chaos_sensor == NULL)
    {
        printk("Device binding problem!\n");
        return;
    }
}


int uchaos_sensor_channel_get(const struct device* dev, enum sensor_channel chan, struct sensor_value* val)
{
    int retVal = z_impl_sensor_channel_get(dev, chan, val);
    chaos_sensorFaultsTypes_t faultType = chaos_getFaultType();

    if (faultType != NONE)
    {
        if (faultType == CONNECTION)
        {
            if (uchaos_sensorConnection())
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


bool uchaos_sensorConnection(void)
{
    static uint32_t eventCounter;
    static uint32_t faultFrequency;
    uint32_t minFaultFreq = (chaos_consoleGetFaultsData() + CONNECTION)->params[0];
    uint32_t maxFaultFreq = (chaos_consoleGetFaultsData() + CONNECTION)->params[1];

    if (minFaultFreq > maxFaultFreq)
    {
        printk("Improper command params order\r\n");
        return false;
    }

    if (faultFrequency == 0)
    {
        faultFrequency = _chaos_generateUIntRandFromRange(minFaultFreq, maxFaultFreq);
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


void uchaos_sensorNoise(struct sensor_value* value)
{
    double measurements[3] = {0};
    // double measurement = 0;

    uint32_t noiseMinPercent = (chaos_consoleGetFaultsData() + NOISE)->params[0];
    uint32_t noiseMaxPercent = (chaos_consoleGetFaultsData() + NOISE)->params[1];

    if (noiseMinPercent > noiseMaxPercent)
    {
        printk("Improper command params order\r\n");
        return;
    }

    uint32_t noisePercent = _chaos_generateUIntRandFromRange(noiseMinPercent, noiseMaxPercent);
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


void uchaos_sensorDataAnomaly(struct sensor_value* value)
{
    static uint32_t eventCounter;
    static uint32_t faultFrequency;
    // double measurement = 0;
    double measurements[3] = {0};
    uint32_t minFaultFreq = (chaos_consoleGetFaultsData() + DATA_ANOMALY)->params[0];
    uint32_t maxFaultFreq = (chaos_consoleGetFaultsData() + DATA_ANOMALY)->params[1];
    uint32_t anomalyMinPercent = (chaos_consoleGetFaultsData() + DATA_ANOMALY)->params[2];
    uint32_t anomalyMaxPercent = (chaos_consoleGetFaultsData() + DATA_ANOMALY)->params[3];


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
        faultFrequency = _chaos_generateUIntRandFromRange(minFaultFreq, maxFaultFreq);
    }

    while (eventCounter < faultFrequency)
    {
        eventCounter++;
        return;
    }
    
    uint32_t anomalyPercent = _chaos_generateUIntRandFromRange(anomalyMinPercent, anomalyMaxPercent);
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


void uchaos_sensorDataSpike(struct sensor_value* value)
{

}


void uchaos_sensorOffset(struct sensor_value* value)
{

}


void uchaos_sensorStuckAtValue(struct sensor_value* value)
{

}
