#include "uchaos_console.h"


K_MSGQ_DEFINE(console_msgq, CHAOS_CONSOLE_MSG_SIZE, CHAOS_CONSOLE_QUEUE_SIZE, 4);
K_THREAD_STACK_DEFINE(chaos_consoleThread_stack_area, CHAOS_CONSOLE_THREAD_STACKSIZE);

const struct device *const chaos_consoleUART = DEVICE_DT_GET(UART_DEV_NODE);
struct k_thread chaos_consoleThreadStruct;
k_tid_t chaos_consoleThread_tid;

static uint8_t _consoleRxBuf[CHAOS_CONSOLE_MSG_SIZE];
static uint16_t _consoleRxBufBytesNbr = 0;

uChaos_Fault_t _faults[] = 
{
    {"none", SENSOR, NONE, 0, NULL},
    {"connection", SENSOR, CONNECTION, 2, NULL},
    {"noise", SENSOR, NOISE, 2, NULL},
    {"data_anomaly", SENSOR, DATA_ANOMALY, 4, NULL},
    {"data_spike", SENSOR, DATA_SPIKE, 3, NULL},
    {"offset", SENSOR, OFFSET, 1, NULL},
    {"stuck_at_value", SENSOR, STUCK_AT_VALUE, 0, NULL},

    {"mem_alloc", MEMORY, MEM_ALLOC, 0, NULL},
    {"mem_free", MEMORY, MEM_FREE, 0, NULL},

    {"load_add", CPU, LOAD_ADD, 0, NULL},
    {"load_del", CPU, LOAD_DEL, 0, NULL},

    {"battery", POWER, BATTERY, 2, NULL},
    {"restart", POWER, RESTART, 0, NULL},
    {"hang_up", POWER, HANG_UP, 0, NULL}
};
uChaos_Fault_t* _currentFault = NULL;
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


uChaos_Fault_t* uChaosConsole_GetFaultsData(void)
{
    return _faults;
}


void uChaosConsole_Init(void)
{
    for (uint8_t i = 0; i < (sizeof(_faults) / sizeof(uChaos_Fault_t)); i++)
    {
        if (_faults[i].paramsNbr > 0)
        {
            _faults[i].params = (uint32_t*)k_malloc(sizeof(uint32_t) * _faults[i].paramsNbr);
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
    for (uint8_t i = 0; i < (sizeof(_faults) / sizeof(uChaos_Fault_t)); i++)
    {
        if (strcmp(_faults[i].name, buf) == 0)
        {
            _currentFault = &_faults[i];
            printk("%s recognized\n", (const char*)_currentFault->name);
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
				if (paramValue && (paramsFound < _faults[_currentSensor->sensorFault.faultType].paramsNbr))
				{
					_faults[_currentSensor->sensorFault.faultType].params[paramsFound] = paramValue;
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

	if (paramsFound ==_currentFault->paramsNbr)
    {
        uChaosSensor_FaultSet(_currentSensor, _currentFault);
        _currentSensor = NULL;
        _currentFault = NULL;
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
    if (!uChaosConsole_SearchForFault(dataBuf))
    {
        if (strcmp("help", buf) == 0)
        {
            uChaosConsole_Help();
            return;
        }
        printk("ERROR: Incorrect command\n");
        return;
    }
    i++;
    uint8_t nextWordStart = i;
    while ((i < (CHAOS_CONSOLE_MSG_SIZE - 1)) && (buf[i] != ' ') && (buf[i] != '\0'))
    {
        dataBuf[i] = buf[i];
        i++;
    }
    if (uChaosConsole_SearchForSensorName(&dataBuf[nextWordStart]))
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
            "- mem_alloc <block_id> <block_bytes_size> <blocks_number>\r\n"
            "- mem_free <block_id>\r\n"
            "--CPU--\r\n"
            "- load_add <thread_name> <thread_priority> <thread_stack_size> <thread_sleep_ms>\r\n"
            "- load_del <thread_name>\r\n"
            "--SUPPLY--\r\n"
            "- battery <voltage_step> <step_interval>\r\n"
            "- restart\r\n"
            "- hang_up\r\n"
    );
}
