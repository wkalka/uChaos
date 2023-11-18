#include <stdio.h>
#include <stdlib.h>

#include "uchaos_battery.h"


#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif


static uChaos_Fault_t _currentFault;
static uint32_t _intervalCounter;
static uint32_t _stepsCounter;


static inline int _uChaosBattery_RawToMillivoltsDt(const struct adc_dt_spec *spec, int32_t *valp)
{
	int32_t vref_mv;
	uint8_t resolution;

	if (!spec->channel_cfg_dt_node_exists)
	{
		return -ENOTSUP;
	}

	if (spec->channel_cfg.reference == ADC_REF_INTERNAL)
	{
		vref_mv = (int32_t)adc_ref_internal(spec->dev);
	}
	else
	{
		vref_mv = spec->vref_mv;
	}

	resolution = spec->resolution;

	if (spec->channel_cfg.differential)
	{
		resolution -= 1U;
	}

	return adc_raw_to_millivolts(vref_mv, spec->channel_cfg.gain, resolution, valp);
}


void uChaosBattery_Init(void)
{
	_currentFault.faultGroup = POWER;
	_currentFault.faultType = NONE;
	_currentFault.params = NULL;
}


int uChaosBattery_RawToMillivoltsDt(const struct adc_dt_spec *spec, int32_t *valp)
{
	int retVal = -ENOTSUP;

	retVal = _uChaosBattery_RawToMillivoltsDt(spec, valp);

	if (_currentFault.faultType != NONE)
	{
		int32_t voltageStep_mV = _currentFault.params[0];
		int32_t stepInterval = _currentFault.params[1];
		int32_t stepsNumber = _currentFault.params[2];
		if ((voltageStep_mV != 0) && (retVal == 0))
		{
			uint32_t stepsMax = (stepsNumber == 0) ? UINT32_MAX : stepsNumber;
			if (_stepsCounter <= stepsMax)
			{
				if (_intervalCounter < stepInterval)
				{ 
					_intervalCounter++;
					if (_intervalCounter == stepInterval)
					{
						_stepsCounter++;
						_intervalCounter = 0;
						if (_stepsCounter > stepsMax)
						{
							_stepsCounter = 0;
							_currentFault.faultType = NONE;
							return retVal;
						}
					}
				}
			}
			*valp = *valp - (_stepsCounter * voltageStep_mV);
			printk("_stepsCounter = %d\r\n", _stepsCounter);
		}
	}

	return retVal;
}


void uChaosBattery_SetFault(uChaos_Fault_t* fault)
{
	if (_currentFault.params == NULL)
	{
		_currentFault.params = (uint32_t*)k_calloc(fault->paramsNbr, sizeof(uint32_t));
	}
	_currentFault.faultType = (fault->faultType == BATTERY_STOP) ? NONE : fault->faultType;
	_intervalCounter = 0;
	_stepsCounter = 0;
	for (uint8_t i = 0; i < fault->paramsNbr; i++)
    {
		_currentFault.params[i] = fault->params[i];
        fault->params[i] = 0;
    }
}
