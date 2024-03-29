#ifndef UCHAOS_BATTERY_H 
#define UCHAOS_BATTERY_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

#include "../uchaos_types.h"
#include "../uchaos_config.h"


#if UCHAOS
#define adc_raw_to_millivolts_dt(a, b)   uChaosBattery_RawToMillivoltsDt(a, b)
#endif

#define BATTERY_INITIAL_VOLTAGE             3.3f
#define BATTERY_DEEP_DISCHARGE_VOLTAGE      2.0f


void uChaosBattery_Init(void);
int uChaosBattery_RawToMillivoltsDt(const struct adc_dt_spec *spec, int32_t *valp);
void uChaosBattery_SetFault(uChaos_Fault_t* fault);

#endif