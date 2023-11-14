#ifndef UCHAOS_MEMORY_H 
#define UCHAOS_MEMORY_H

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


#define MEMORY_BLOCKS_NUMBER      4


typedef struct
{
    uint8_t blockID;
    uint8_t* blockPointer
} uChaosMemory_t;


void uChaosMemory_Init(void);

void uChaosMemory_BlockAlloc(uint8_t blockID, uint32_t bytesNumber);
void uChaosMemory_BlockFree(uint8_t blockID);

#endif