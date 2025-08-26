// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jdsimple.h"
#include "persistent.h"

// Include HAL Flash headers
#include "stm32f0xx_hal_flash.h"
#include "stm32f0xx_hal_flash_ex.h"
#include <stdint.h>

// Use the last page of flash (page 31 for STM32F030 with 32KB flash)
// Each page is 1KB (1024 bytes)
#define PERSISTENT_DATA_ADDRESS 0x08007C00  // Last page start (32KB - 1KB = 31KB = 0x7C00)
#define FIRST_BOOT_FLAG_OFFSET  0x00        // Offset within the page
#define FIRST_BOOT_MAGIC        0xDEADBEEF  // Magic value indicating first boot complete

// Structure for persistent data
typedef struct {
    uint32_t first_boot_magic;
    uint32_t padding[255];  // Fill rest of page to prevent accidental writes
} persistent_data_t;

static volatile persistent_data_t* persistent_data = (persistent_data_t*)PERSISTENT_DATA_ADDRESS;

// Check if this is the first boot
bool is_first_boot(void) {
    return (persistent_data->first_boot_magic != FIRST_BOOT_MAGIC);
}

// Mark first boot as complete
void mark_first_boot_complete(void) {
    HAL_StatusTypeDef status;
    
    // Unlock flash
    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) {
        DMESG("Flash unlock failed: %d", status);
        return;
    }
    
    // Erase the page
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;
    
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = PERSISTENT_DATA_ADDRESS;
    erase_init.NbPages = 1;
    
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    if (status != HAL_OK) {
        DMESG("Flash erase failed: %d, page_error: %d", status, page_error);
        HAL_FLASH_Lock();
        return;
    }
    
    // Program the magic value
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, 
                               PERSISTENT_DATA_ADDRESS + FIRST_BOOT_FLAG_OFFSET,
                               FIRST_BOOT_MAGIC);
    
    if (status != HAL_OK) {
        DMESG("Flash program failed: %d", status);
    } else {
        DMESG("First boot flag written successfully");
    }
    
    // Lock flash
    HAL_FLASH_Lock();
}
