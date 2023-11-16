#include "uchaos_console.h"


K_MSGQ_DEFINE(uChaosConsole_Msgq, UCHAOS_CONSOLE_MSG_SIZE, UCHAOS_CONSOLE_QUEUE_SIZE, 4);
K_THREAD_STACK_DEFINE(uChaosConsole_ThreadStackArea, UCHAOS_CONSOLE_THREAD_STACKSIZE);

const struct device *const uChaosConsole_UART = DEVICE_DT_GET(UART_DEV_NODE);
struct k_thread uChaosConsole_ThreadStruct;
k_tid_t uChaosConsole_Thread_tid;

static uint8_t _uChaosConsole_RxBuf[UCHAOS_CONSOLE_MSG_SIZE];
static uint16_t _uChaosConsole_RxBufBytesCount = 0;

static uChaos_Fault_t _faults[] = 
{
    {"none", SENSOR, NONE, 0, NULL},
    {"connection", SENSOR, CONNECTION, 2, NULL},
    {"noise", SENSOR, NOISE, 2, NULL},
    {"data_anomaly", SENSOR, DATA_ANOMALY, 4, NULL},
    {"data_spike", SENSOR, DATA_SPIKE, 3, NULL},
    {"offset", SENSOR, OFFSET, 1, NULL},
    {"stuck_at_value", SENSOR, STUCK_AT_VALUE, 0, NULL},

    {"mem_alloc", MEMORY, MEM_ALLOC, 0, NULL},
    {"mem_free", MEMORY, NONE, 0, NULL},

    {"load_add", CPU, LOAD_ADD, 0, NULL},
    {"load_del", CPU, NONE, 0, NULL},

    {"battery", POWER, BATTERY, 3, NULL},
    {"battery_stop", POWER, NONE, 0, NULL},
    {"restart", POWER, RESTART, 0, NULL},
    {"hang_up", POWER, HANG_UP, 0, NULL}
};
static uChaos_Fault_t* _currentFault = NULL;


static void _uChaosConsole_ClearRxBuff(void)
{
    memset(_uChaosConsole_RxBuf, 0, sizeof(_uChaosConsole_RxBuf));
    _uChaosConsole_RxBufBytesCount = 0;
}


static void _uChaosConsole_Callback(const struct device *dev, void *userData)
{
    uint8_t c = 0;
	uart_irq_update(dev);

    while (uart_irq_rx_ready(dev))
    {
        uart_fifo_read(dev, &c, 1);

        if (((c == '\r') || (c == '\n')) && (_uChaosConsole_RxBufBytesCount > 0))
        {
            _uChaosConsole_RxBuf[_uChaosConsole_RxBufBytesCount] = '\0';
            k_msgq_put(&uChaosConsole_Msgq, &_uChaosConsole_RxBuf, K_NO_WAIT);
            _uChaosConsole_ClearRxBuff();
        }
        else if (_uChaosConsole_RxBufBytesCount < (sizeof(_uChaosConsole_RxBuf) - 1))
        {
            _uChaosConsole_RxBuf[_uChaosConsole_RxBufBytesCount++] = c;
        }
    }
}


void uChaosConsole_Init(void)
{
    for (uint8_t i = 0; i < (sizeof(_faults) / sizeof(uChaos_Fault_t)); i++)
    {
        if (_faults[i].paramsNbr > 0)
        {
            _faults[i].params = (uint32_t*)k_calloc(_faults[i].paramsNbr, sizeof(uint32_t));
        }
    }

    uart_irq_callback_user_data_set(uChaosConsole_UART, _uChaosConsole_Callback, NULL);
	uart_irq_rx_enable(uChaosConsole_UART);

    uChaosConsole_Thread_tid = k_thread_create(&uChaosConsole_ThreadStruct, uChaosConsole_ThreadStackArea,
                                                K_THREAD_STACK_SIZEOF(uChaosConsole_ThreadStackArea),
                                                uChaosConsole_Thread,
                                                NULL, NULL, NULL,
                                                UCHAOS_CONSOLE_THREAD_PRIORITY, 0, K_NO_WAIT);
}


void uChaosConsole_Thread(void* arg1, void* arg2, void* arg3)
{
    uChaosConsole_ThreadFunction(UCHAOS_CONSOLE_THREAD_SLEEP, 0);
}


void uChaosConsole_ThreadFunction(uint32_t sleep_ms, uint32_t id)
{
    uint8_t dataBuf[UCHAOS_CONSOLE_MSG_SIZE] = {0};

	while (k_msgq_get(&uChaosConsole_Msgq, &dataBuf, K_FOREVER) == 0)
    {
		printk("Data received: ");
        printk("%s", (const char*)dataBuf); 
        printk("\r\n");
        uChaosConsole_CheckCommand(dataBuf);
	}
}


bool uChaosConsole_SearchForFault(uint8_t* buf)
{
    for (uint8_t i = 0; i < (sizeof(_faults) / sizeof(uChaos_Fault_t)); i++)
    {
        if (strcmp(_faults[i].name, buf) == 0)
        {
            _currentFault = &_faults[i];
            printk("%s recognized\r\n", (const char*)_currentFault->name);
            return true;
        }
    }

    return false;
}


bool uChaosConsole_SearchForStringParam(uint8_t* destination, uint8_t* source, uint8_t* index)
{
    uint8_t i = 0;
    bool retVal = false;
    while ((i < UCHAOS_CONSOLE_MSG_SIZE) && (source[i] != ' ') && (source[i] != '\0'))
    {
        destination[i] = source[i];
        i++;
        (*index)++;
    }

    switch (_currentFault->faultGroup)
    {
        case SENSOR:
        {
            retVal = uChaosConsole_SearchForSensorName(&destination[0]);
            break;
        }
        case CPU:
        {
            retVal = uChaosConsole_SearchForThreadName(&destination[0]);
            break;
        }
        case MEMORY:
        case POWER:
        {
            retVal = true;
            break;
        }
        default:
        {
            retVal = false;
            break;
        }
    }

    return retVal;
}


bool uChaosConsole_SearchForSensorName(uint8_t* buf)
{
    for (uint8_t i = 0;  i < UCHAOS_SENSORS_NUMBER; i++)
    {
        if (strcmp((uChaosSensor_GetSensors() + i)->name, buf) == 0)
        {
            uChaosSensor_SetCurrentSensor((uChaosSensor_GetSensors() + i));
            printk("Sensor recognized: %s\r\n", (const char*)(uChaosSensor_GetSensors() + i)->name);
            return true;
        }
    }
    return false;
}


bool uChaosConsole_SearchForThreadName(uint8_t* buf)
{
    for (uint8_t i = 0;  i < UCHAOS_SENSORS_NUMBER; i++)
    {
        // if (strcmp((uChaosSensor_GetSensors() + i)->name, buf) == 0)
        // {
        //     _currentSensor = (uChaosSensor_GetSensors() + i);
        //     printk("Sensor recognized: %s\n", (const char*)_currentSensor->name);
        //     return true;
        // }
    }
    return false;
}


bool uChaosConsole_SetFault(uChaos_Fault_t* fault)
{
    if (fault == NULL)
    {
        printk("ERROR: Can not set current fault - null pointer\r\n");
        return false;
    }

    switch (fault->faultGroup)
    {
        case SENSOR:
        {
            uChaosSensor_SetFault(_currentFault);
            uChaosSensor_SetCurrentSensor(NULL);
            break;
        }
        case MEMORY:
        {
            break;
        }
        case CPU:
        {
            break;
        }
        case POWER:
        {
            break;
        }
        default:
        {
            return false;
        }
    }

    return true;
}


bool uChaosConsole_ParseCommand(uint8_t* buf, uint8_t* index)
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
				if (paramValue && (paramsFound < _faults[_currentFault->faultType].paramsNbr))
				{
					_faults[_currentFault->faultType].params[paramsFound] = paramValue;
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
        (*index)++;
		if (*index == UCHAOS_CONSOLE_MSG_SIZE) { break; }
	}

	if (paramsFound ==_currentFault->paramsNbr)
    {
        if (uChaosConsole_SetFault(_currentFault))
        {
            _currentFault = NULL;
            return true;
        }
        return false;
    }
	else
    { 
        return false; 
    }
}


void uChaosConsole_CheckCommand(uint8_t* buf)
{
    uint8_t dataBuf[UCHAOS_CONSOLE_MSG_SIZE] = {0};
    uint8_t i = 0;

    while ((i < UCHAOS_CONSOLE_MSG_SIZE) && (buf[i] != ' ') && (buf[i] != '\0'))
    {
        dataBuf[i] = buf[i];
        i++;
    }
    if (!uChaosConsole_SearchForFault(dataBuf))
    {
        if (strcmp("help", buf) == 0)
        {
            uChaosConsole_Help();
            return;
        }
        printk("ERROR: Incorrect command\r\n");
        return;
    }
    i++;
    if (uChaosConsole_SearchForStringParam(&dataBuf[i], &buf[i], &i))
    {
        i++;
        if (!uChaosConsole_ParseCommand(&buf[i], &i))
        {
            printk("ERROR: Incorrect command\r\n");
            return;
        }
    }
    else
    {
        printk("ERROR: Incorrect command\r\n");
        return;
    }
}


void uChaosConsole_Help(void)
{
    printk("\r\n\r\nuChaos help: commands and parameters\r\n"
            "--SENSOR--\r\n"
            "- none <sensor_name>\r\n"
            "- connection <sensor_name> <min_frequency> <max_frequency>\r\n"
            "- noise <sensor_name> <min_level> <max_level>\r\n"
            "- data_anomaly <sensor_name> <min_frequency> <max_frequency> <min_level> <max_level>\r\n"
            "- data_spike <sensor_name> <direction> <max_level> <samples_length>\r\n"
            "- offset <sensor_name> <direction> <level>\r\n"
            "- stuck_at_value <sensor_name>\r\n"
            "--MEMORY--\r\n"
            "- mem_alloc <block_id> <block_bytes_size>\r\n"
            "- mem_free <block_id>\r\n"
            "--CPU--\r\n"
            "- load_add <thread_name>\r\n"
            "- load_del <thread_name>\r\n"
            "- restart\r\n"
            "- hang_up\r\n"
            // "- load_add <thread_name> <thread_priority> <thread_stack_size> <thread_sleep_ms>\r\n"
            // "- load_del <thread_name>\r\n"
            "--POWER--\r\n"
            "- battery <voltage_step_mV> <step_interval> <steps_number>\r\n"
            "- battery_stop\r\n"
    );
}
