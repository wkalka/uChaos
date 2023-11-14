#include <stdio.h>
#include <stdlib.h>

#include "uchaos_battery.h"


#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif


static int32_t _voltageStep_mV;
static int32_t _stepInterval;
static int32_t _stepsNumber;


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


int uChaosBattery_RawToMillivoltsDt(const struct adc_dt_spec *spec, int32_t *valp)
{
	static uint32_t intervalCounter;
	static uint32_t stepsCounter;
	int retVal = -ENOTSUP;

	retVal = _uChaosBattery_RawToMillivoltsDt(spec, valp);

	if ((_voltageStep_mV != 0) && (retVal == 0))
	{
		uint32_t stepsMax = (_stepsNumber == 0) ? UINT32_MAX : _stepsNumber;
		if ( _stepsNumber == 0) // tryb ciągły
		{
			if (stepsCounter < stepsMax)
			{
				if (intervalCounter < _stepInterval)
				{ 
					intervalCounter++;
				}
				else
				{
					stepsCounter++;
					intervalCounter = 0;
				}
			}
		}
		*valp = *valp - (stepsCounter * _voltageStep_mV);
	}

	return retVal;
}


void uChaosBattery_FaultSet(uChaos_Fault_t *fault)
{
	_voltageStep_mV = fault->params[0];
	_stepInterval = fault->params[1];
	_stepsNumber = fault->params[2];
}


void uChaosBattery_FaultStop(void)
{
	_voltageStep_mV = 0;
	_stepInterval = 0;
	_stepsNumber = 0;
}