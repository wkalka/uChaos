#include "uchaos_console.h"


K_MSGQ_DEFINE(console_msgq, CHAOS_CONSOLE_MSG_SIZE, CHAOS_CONSOLE_QUEUE_SIZE, 4);
K_THREAD_STACK_DEFINE(chaos_consoleThread_stack_area, CHAOS_CONSOLE_THREAD_STACKSIZE);

const struct device *const chaos_consoleUART = DEVICE_DT_GET(UART_DEV_NODE);
struct k_thread chaos_consoleThreadStruct;
k_tid_t chaos_consoleThread_tid;

uChaos_SensorFault_t _sensorFaults[] = 
{
    {"none", NONE, 0, NULL},
    {"connection", CONNECTION, 2, NULL},
    {"noise", NOISE, 2, NULL},
    {"data_anomaly", DATA_ANOMALY, 4, NULL},
    {"data_spike", DATA_SPIKE, 0, NULL},
    {"offset", OFFSET, 1, NULL},
    {"stuck_at_value", STUCK_AT_VALUE, 1, NULL}
};
uChaos_SensorFault_t* sensorFaults = _sensorFaults;
static uint8_t _consoleRxBuf[CHAOS_CONSOLE_MSG_SIZE];
static uint16_t _consoleRxBufBytesNbr = 0;
uChaosSensor_t* _currentSensor = NULL;


static void uChaosConsole_ClearRxBuff(void)
{
    memset(_consoleRxBuf, 0, sizeof(_consoleRxBuf));
    _consoleRxBufBytesNbr = 0;
}


static void uChaosConsole_Callback(const struct device *dev, void *userData)
{
    uint8_t c = 0;
	uart_irq_update(dev);

    while (uart_irq_rx_ready(dev))
    {
        uart_fifo_read(dev, &c, 1);

        if (((c == '\r') || (c == '\n')) && (_consoleRxBufBytesNbr > 0))
        {
            _consoleRxBuf[_consoleRxBufBytesNbr] = '\0';
            k_msgq_put(&console_msgq, &_consoleRxBuf, K_NO_WAIT);
            uChaosConsole_ClearRxBuff();
        }
        else if (_consoleRxBufBytesNbr < (sizeof(_consoleRxBuf) - 1))
        {
            _consoleRxBuf[_consoleRxBufBytesNbr++] = c;
        }
    }
}


uChaos_SensorFault_t* uChaosConsole_GetFaultsData(void)
{
    return sensorFaults;
}

void uChaosConsole_Init(void)
{
    for (uint8_t i = 0; i < (sizeof(_sensorFaults) / sizeof(uChaos_SensorFault_t)); i++)
    {
        if (_sensorFaults[i].paramsNbr > 0)
        {
            _sensorFaults[i].params = (uint32_t*)k_malloc(sizeof(uint32_t) * _sensorFaults[i].paramsNbr);
        }
    }

    uart_irq_callback_user_data_set(chaos_consoleUART, uChaosConsole_Callback, NULL);
	uart_irq_rx_enable(chaos_consoleUART);

    chaos_consoleThread_tid = k_thread_create(&chaos_consoleThreadStruct, chaos_consoleThread_stack_area,
                                                K_THREAD_STACK_SIZEOF(chaos_consoleThread_stack_area),
                                                uChaosConsole_Thread,
                                                NULL, NULL, NULL,
                                                K_LOWEST_THREAD_PRIO, 0, K_NO_WAIT);
}


void uChaosConsole_Thread(void* arg1, void* arg2, void* arg3)
{
    uChaosConsole_ThreadFunction(CHAOS_CONSOLE_THREAD_SLEEP, 0);
}


void uChaosConsole_ThreadFunction(uint32_t sleep_ms, uint32_t id)
{
    uint8_t dataBuf[CHAOS_CONSOLE_MSG_SIZE] = {0};

	while (k_msgq_get(&console_msgq, &dataBuf, K_FOREVER) == 0)
    {
		printk("Data received: ");
        printk("%s", (const char*)dataBuf); 
        printk("\r\n");
        uChaosConsole_CheckCommand(dataBuf);
	}
}


bool uChaosConsole_SearchForFault(uint8_t* buf)
{
    for (uint8_t i = 0; i < (sizeof(_sensorFaults) / sizeof(uChaos_SensorFault_t)); i++)
    {
        if (strcmp(_sensorFaults[i].name, buf) == 0)
        {
            _currentSensor->sensorFault.faultType = _sensorFaults[i].faultType;
            memset(_currentSensor->sensorFault.name, 0, UCHAOS_FAULT_NAME_LEN);
            snprintf(_currentSensor->sensorFault.name, UCHAOS_FAULT_NAME_LEN, "%s", (const char*)_sensorFaults[i].name);
            printk("%s recognized\n", (const char*)_currentSensor->sensorFault.name);
            return true;
        }
    }

    return false;
}

bool uChaosConsole_ParseCommand(uint8_t* buf)
{
	int32_t paramValue = 0;
    uint8_t paramDigits[3] = {0};
	uint8_t digitsCount = 0;
	uint8_t paramsFound = 0;
	uint8_t i = 0;

	while (true)
	{
		if ((buf[i] == ' ') || (buf[i] == '\0'))
		{
			if (digitsCount > 0)
			{
				paramValue = (int32_t)atoi((const char*)paramDigits);
				if (paramValue && (paramsFound < _sensorFaults[_currentSensor->sensorFault.faultType].paramsNbr))
				{
					_sensorFaults[_currentSensor->sensorFault.faultType].params[paramsFound] = paramValue;
				}
				paramsFound++;
				digitsCount = 0;
				memset(paramDigits, 0, sizeof(paramDigits));
				if (buf[i] == '\0') { break; }
			}
            else if ((buf[i] == '\0') && (i == 0))
            {
                break;
            }
		}
		else
		{
			if ((digitsCount < sizeof(paramDigits)) && (buf[i] >= DIGITS_ASCII_START) && (buf[i] <= DIGITS_ASCII_END))
			{
				paramDigits[digitsCount] = buf[i];
				digitsCount++;
			}
		}
		i++;
		if (i == CHAOS_CONSOLE_MSG_SIZE) { break; } // todo: not compare to CHAOS_CONSOLE_MSG_SIZE
	}

	if (paramsFound == _sensorFaults[_currentSensor->sensorFault.faultType].paramsNbr)
    {
        uChaosSensor_FaultSet(_currentSensor, &_sensorFaults[_currentSensor->sensorFault.faultType]);
        _currentSensor = NULL;
        return true;
    }
	else { return false; }
}

bool uChaosConsole_SearchForSensorName(uint8_t* buf)
{
    for (uint8_t i = 0;  i < UCHAOS_SENSORS_NUMBER; i++)
    {
        if (strcmp((uChaosSensor_GetSensor() + i)->name, buf) == 0)
        {
            _currentSensor = (uChaosSensor_GetSensor() + i);
            printk("Sensor recognized: %s\n", (const char*)_currentSensor->name);
            return true;
        }
    }
    return false;
}

void uChaosConsole_CheckCommand(uint8_t* buf)
{
    uint8_t dataBuf[CHAOS_CONSOLE_MSG_SIZE] = {0};
    uint8_t i = 0;

    while ((i < (CHAOS_CONSOLE_MSG_SIZE - 1)) && (buf[i] != ' ') && (buf[i] != '\0'))
    {
        dataBuf[i] = buf[i];
        i++;
    }
    if (!uChaosConsole_SearchForSensorName(dataBuf))
    {
        printk("ERROR: Sensor name not found\n");
        return;
    }
    i++;
    uint8_t nextWordStart = i;
    while ((i < (CHAOS_CONSOLE_MSG_SIZE - 1)) && (buf[i] != ' ') && (buf[i] != '\0'))
    {
        dataBuf[i] = buf[i];
        i++;
    }
    if (uChaosConsole_SearchForFault(&dataBuf[nextWordStart]))
    {
        if (!uChaosConsole_ParseCommand(&buf[i]))
        {
            printk("ERROR: Incorrect command\n");    
        }
    }
    else
    {
        printk("ERROR: Incorrect command\n");
    }
}

uChaos_SensorFaultsTypes_t chaos_getFaultType(void)
{
    if (_currentSensor != NULL)
    {
        return _currentSensor->sensorFault.faultType;
    }
    else
    {
        return NONE;
    }
}
