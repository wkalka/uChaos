#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/rand32.h>
#include <zephyr/kernel.h>

#include "uchaos/uchaos.h"

#include <stdlib.h>
#include <stdio.h>

#define ADXL345

#ifdef ADXL345
#define SLEEP_MS	500
#endif
#ifdef DPS310
#define SLEEP_MS	2000
#endif


int main(void)
{
	printk("Demo Application start\n\n");

#ifdef ADXL345
    struct sensor_value sensorValue[3] = {0};
    double measurement[3] = {0};
	const struct device *const adxl345 = DEVICE_DT_GET_ONE(adi_adxl345);
#endif

#ifdef DPS310
    struct sensor_value sensorValue = {0};
    double measurement = 0;

	const struct device *const dev = DEVICE_DT_GET_ONE(infineon_dps310);
#endif

    uint32_t counter = 1;
	chaos_consoleInit();
    uChaosSensor_Init("adxl345", adxl345);

	while (1)
    {
        if (sensor_sample_fetch(adxl345) < 0)
        {
            printk("Sample fetch error!\n");
        }
#ifdef ADXL345
        if (sensor_channel_get(adxl345, SENSOR_CHAN_ACCEL_XYZ, &sensorValue[0]) < 0)
        {
            printk("Channel get error!\n");
        }
        measurement[0] = sensor_value_to_double(&sensorValue[0]);
        measurement[1] = sensor_value_to_double(&sensorValue[1]);
        measurement[2] = sensor_value_to_double(&sensorValue[2]);
        printk("%d X = %f;Y = %f;Z = %f\r\n", counter++, measurement[0], measurement[1], measurement[2]);
#endif
#ifdef DPS310
        if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &sensorValue) < 0)
        {
            printk("Channel get error!\n");
        }
        measurement = sensor_value_to_double(&sensorValue);
        printk("%d Temp = %f\r\n", counter++, measurement);
#endif        
        k_sleep(K_MSEC(SLEEP_MS));
	}
	return 0;
}
