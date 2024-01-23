#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/rand32.h>

#include "uchaos/uchaos.h"

#include <stdlib.h>
#include <stdio.h>

// #define CPU_LOAD
// #define ADC_VBATT
// #define ADXL345
// #define DPS310

#ifdef CPU_LOAD
#define SLEEP_MS	5000
#endif
#ifdef ADC_VBATT
#define SLEEP_MS	10000
#endif
#ifdef ADXL345
#define SLEEP_MS	2000
#endif
#ifdef DPS310
#define SLEEP_MS	2000
#endif


K_THREAD_STACK_DEFINE(_threadStack, 1024);
static struct k_thread _thread;
static k_tid_t _threadID;

static void _threadFunction(void* arg1, void* arg2, void* arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint32_t counter = 1;
    // struct sensor_value sensorValue[3] = {0};
    // double measurement[3] = {0};
    struct sensor_value sensorValue = {0};
    double measurement = 0;
	// const struct device *const dev = DEVICE_DT_GET_ONE(adi_adxl345);
    const struct device *const dev = DEVICE_DT_GET_ONE(infineon_dps310);
    // if (!uChaosSensor_Create("adxl345", dev))
    if (!uChaosSensor_Create("dps310", dev))
    {
        return;
    }
    int64_t lasTime = 0;
    int64_t nowTime = 0;
    uint64_t totalTime = 0;

	while (1)
    {
        if (sensor_sample_fetch(dev) < 0)
        {
            printk("Sample fetch error!\r\n");
        }
        // if (sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, &sensorValue[0]) < 0)
        if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &sensorValue) < 0)
        {
            printk("Channel get error!\r\n");
        }
        // measurement[0] = sensor_value_to_double(&sensorValue[0]);
        // measurement[1] = sensor_value_to_double(&sensorValue[1]);
        // measurement[2] = sensor_value_to_double(&sensorValue[2]);
        measurement = sensor_value_to_double(&sensorValue);
        nowTime = k_uptime_get();
        totalTime += (nowTime-lasTime);
        // printk("%d;%f;%f;%f;%lld;%lld\r\n", counter, measurement[0], measurement[1], measurement[2], (nowTime-lasTime), totalTime);
        printk("%d;%f;%lld;%lld\r\n", counter, measurement, (nowTime-lasTime), totalTime);
        lasTime = nowTime;
        counter++;
        k_sleep(K_MSEC(2000));
	}
}

static void thread(void* arg1, void* arg2, void* arg3)
{
    _threadFunction(arg1, arg2, arg3);
}


int main(void)
{
	printk("Demo Application start\n\n");

    uChaosConsole_Init();

#ifdef CPU_LOAD   
    uChaosCPU_Init();
    _threadID = k_thread_create(&_thread, _threadStack,
                                K_THREAD_STACK_SIZEOF(_threadStack),
                                thread,
                                NULL, NULL, NULL,
                                12, 0, K_NO_WAIT);
#endif
#ifdef ADC_VBATT
    uChaosBattery_Init();
#endif

#ifdef ADC_VBATT
    uint32_t counter = 1;
    int32_t val_mv;
    int error;
	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
	};
    static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

    if (!adc_is_ready_dt(&adc_channel)) 
    {
        printk("ADC controller device %s not ready\r\n", adc_channel.dev->name);
        return 0;
    }
    error = adc_channel_setup_dt(&adc_channel);
    if (error < 0)
    {
        printk("Could not setup channel - error %d\r\n", error);
        return 0;
    }
#endif

#ifdef ADXL345
    uint32_t counter = 1;
    struct sensor_value sensorValue[3] = {0};
    double measurement[3] = {0};
	const struct device *const adxl345 = DEVICE_DT_GET_ONE(adi_adxl345);
    if (!uChaosSensor_Create("adxl345", adxl345))
    {
        return 0;
    }
#endif

#ifdef DPS310
    uint32_t counter = 1;
    struct sensor_value sensorValue = {0};
    double measurement = 0;
	const struct device *const dps310 = DEVICE_DT_GET_ONE(infineon_dps310);
#endif

	while (1)
    {
#ifdef ADC_VBATT
        adc_sequence_init_dt(&adc_channel, &sequence);
        error = adc_read(adc_channel.dev, &sequence);
        if (error < 0) {
            printk("Could not read - error %d\r\n", error);
            continue;
        }
        val_mv = (int32_t)buf;
        error = adc_raw_to_millivolts_dt(&adc_channel, &val_mv);
        printk("%d;%"PRId32"\r\n", counter, val_mv);
        counter++;
        k_sleep(K_MSEC(SLEEP_MS));
#endif
#ifdef ADXL345
        if (sensor_sample_fetch(adxl345) < 0)
        {
            printk("Sample fetch error!\r\n");
        }
        if (sensor_channel_get(adxl345, SENSOR_CHAN_ACCEL_XYZ, &sensorValue[0]) < 0)
        {
            printk("Channel get error!\r\n");
        }
        measurement[0] = sensor_value_to_double(&sensorValue[0]);
        measurement[1] = sensor_value_to_double(&sensorValue[1]);
        measurement[2] = sensor_value_to_double(&sensorValue[2]);
        printk("%d;%f;%f;%f\r\n", counter, measurement[0], measurement[1], measurement[2]);
        counter++;
        k_sleep(K_MSEC(SLEEP_MS));
#endif
#ifdef DPS310
        if (sensor_sample_fetch(dps310) < 0)
        {
            printk("Sample fetch error!\r\n");
        }
        if (sensor_channel_get(dps310, SENSOR_CHAN_AMBIENT_TEMP, &sensorValue) < 0)
        {
            printk("Channel get error!\r\n");
        }
        measurement = sensor_value_to_double(&sensorValue);
        printk("%d;%f\r\n", counter, measurement);
        counter++;
        k_sleep(K_MSEC(SLEEP_MS));
#endif
#ifdef CPU_LOAD
        k_sleep(K_MSEC(SLEEP_MS));
#endif
	}

	return 0;
}
