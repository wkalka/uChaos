#include "uchaos_console.h"

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

K_MSGQ_DEFINE(console_msgq, CHAOS_CONSOLE_MSG_SIZE, CHAOS_CONSOLE_QUEUE_SIZE, 4);
K_THREAD_STACK_DEFINE(chaos_consoleThread_stack_area, CHAOS_CONSOLE_THREAD_STACKSIZE);

const struct device *const chaos_consoleUART = DEVICE_DT_GET(UART_DEV_NODE);
struct k_thread chaos_consoleThreadStruct;
k_tid_t chaos_consoleThread_tid;

static uint8_t consoleRxBuf[CHAOS_CONSOLE_MSG_SIZE];
static uint16_t consoleRxBufBytesNbr = 0;
uChaosSensor_t* currentSensor = NULL;

uChaos_SensorFault_t* sensorFaults = _sensorFaults;

static void _chaos_consoleCallback(const struct device *dev, void *userData)
{
    uint8_t c = 0;
	uart_irq_update(dev);

    while (uart_irq_rx_ready(dev))
    {
        uart_fifo_read(dev, &c, 1);

        if (((c == '\r') || (c == '\n')) && (consoleRxBufBytesNbr > 0))
        {
            consoleRxBuf[consoleRxBufBytesNbr] = '\0';
            k_msgq_put(&console_msgq, &consoleRxBuf, K_NO_WAIT);
            consoleRxBufBytesNbr = 0;
        }
        else if (consoleRxBufBytesNbr < (sizeof(consoleRxBuf) - 1))
        {
            consoleRxBuf[consoleRxBufBytesNbr++] = c;
        }
    }
}


uChaos_SensorFault_t* chaos_consoleGetFaultsData(void)
{
    return sensorFaults;
}

void chaos_consoleInit(void)
{
    for (uint8_t i = 0; i < (sizeof(_sensorFaults) / sizeof(uChaos_SensorFault_t)); i++)
    {
        if (_sensorFaults[i].paramsNbr > 0)
        {
            _sensorFaults[i].params = (uint32_t*)k_malloc(sizeof(uint32_t) * _sensorFaults[i].paramsNbr);
        }
    }

    uart_irq_callback_user_data_set(chaos_consoleUART, _chaos_consoleCallback, NULL);
	uart_irq_rx_enable(chaos_consoleUART);

    chaos_consoleThread_tid = k_thread_create(&chaos_consoleThreadStruct, chaos_consoleThread_stack_area,
                                                K_THREAD_STACK_SIZEOF(chaos_consoleThread_stack_area),
                                                chaos_consoleThread,
                                                NULL, NULL, NULL,
                                                K_LOWEST_THREAD_PRIO, 0, K_NO_WAIT);
}


void chaos_consoleThread(void* arg1, void* arg2, void* arg3)
{
    chaos_consoleThreadFunction(CHAOS_CONSOLE_THREAD_SLEEP, 0);
}


void chaos_consoleThreadFunction(uint32_t sleep_ms, uint32_t id)
{
    uint8_t dataBuf[CHAOS_CONSOLE_MSG_SIZE] = {0};

	while (k_msgq_get(&console_msgq, &dataBuf, K_FOREVER) == 0)
    {
		printk("Data received: ");
        printk("%s", (const char*)dataBuf); 
        printk("\r\n");
        chaos_consoleCheckCommand(dataBuf);
	}
}


bool chaos_consoleSearchForCommand(uint8_t* buf)
{
    for (uint8_t i = 0; i < (sizeof(_sensorFaults) / sizeof(uChaos_SensorFault_t)); i++)
    {
        if (strcmp(_sensorFaults[i].name, buf) == 0)
        {
            currentSensor->sensorFault.faultType = _sensorFaults[i].faultType;
            printk("%s recognized\n", (const char*)_sensorFaults[i].name);
            return true;
        }
    }

    return false;
}

bool chaos_consoleParseCommand(uint8_t* buf)
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
				if (paramValue && (paramsFound < _sensorFaults[currentSensor->sensorFault.faultType].paramsNbr))
				{
					_sensorFaults[currentSensor->sensorFault.faultType].params[paramsFound] = paramValue;
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
		if (i == CHAOS_CONSOLE_MSG_SIZE) { break; }
	}

	if (paramsFound == _sensorFaults[currentSensor->sensorFault.faultType].paramsNbr)
    {
        currentSensor->sensorFault = _sensorFaults[currentSensor->sensorFault.faultType];
        return true;
    }
	else { return false; }
}

bool chaos_consoleSearchForSensorName(uint8_t* buf)
{
    for (uint8_t i = 0;  i < UCHAOS_SENSORS_NUMBER; i++)
    {
        if (strcmp((uChaosSensor_GetSensor() + i)->name, buf) == 0)
        {
            currentSensor = (uChaosSensor_GetSensor() + i);
            printk("Sensor recognized: %s\n", (const char*)currentSensor->name);
            return true;
        }
    }
    return false;
}

void chaos_consoleCheckCommand(uint8_t* buf)
{
    uint8_t dataBuf[CHAOS_CONSOLE_MSG_SIZE] = {0};
    uint8_t i = 0;

    while ((i < (CHAOS_CONSOLE_MSG_SIZE - 1)) && (buf[i] != ' ') && (buf[i] != '\0'))
    {
        dataBuf[i] = buf[i];
        i++;
    }
    if (!chaos_consoleSearchForSensorName(dataBuf))
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
    if (chaos_consoleSearchForCommand(&dataBuf[nextWordStart]))
    {
        if (!chaos_consoleParseCommand(&buf[i]))
        {
            printk("ERROR: Incorrect command\n");    
        }
    }
    else
    {
        printk("ERROR: Incorrect command\n");
    }
}

void chaos_clearConsoleRxBuff(void)
{
    memset(consoleRxBuf, 0, sizeof(consoleRxBuf));
    consoleRxBufBytesNbr = 0;
}


uChaos_SensorFaultsTypes_t chaos_getFaultType(void)
{
    if (currentSensor != NULL)
    {
        return currentSensor->sensorFault.faultType;
    }
    else
    {
        return NONE;
    }
}
