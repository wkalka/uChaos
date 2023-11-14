#include "uchaos_memory.h"


static uChaosMemory_t _uChaosMemory[MEMORY_BLOCKS_NUMBER];


void uChaosMemory_Init(void)
{
    for (uint8_t i = 0; i < MEMORY_BLOCKS_NUMBER; i++)
    {
        _uChaosMemory[i].blockID = i + 1;
        _uChaosMemory[i].blockPointer = NULL;
    }
}


void uChaosMemory_BlockAlloc(uint8_t blockID, uint32_t bytesNumber)
{
    if ( _uChaosMemory[blockID].blockPointer != NULL )
    {
        printk("WARNING: Block %d already allocated\r\n", _uChaosMemory[blockID].blockID);
        return;
    }
    _uChaosMemory[blockID].blockPointer = (uint8_t*)k_calloc(bytesNumber, sizeof(uint8_t));
    if ( _uChaosMemory[blockID].blockPointer == NULL )
    {
        printk("ERROR: Can't allocate block %d with size %d\r\n", blockID, bytesNumber);
    }
}


void uChaosMemory_BlockFree(uint8_t blockID)
{
    if ( _uChaosMemory[blockID].blockPointer == NULL )
    {
        printk("WARNING: Block %d isn't allocated\r\n");
        return;
    }
    k_free(_uChaosMemory[blockID].blockPointer);
    _uChaosMemory[blockID].blockPointer = NULL;
}