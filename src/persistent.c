// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jdsimple.h"
#include "persistent.h"

// Include HAL Flash headers
#include "stm32f0xx_hal_flash.h"
#include "stm32f0xx_hal_flash_ex.h"
#include <stdint.h>

// Use the last page of flash (page 31 for STM32F030C8 with 64KB flash)
// Each page is 1KB (1024 bytes)
// STM32F030C8: 64KB flash = 0x10000 bytes, pages 0-63, last page at 0x0800FC00
#define PERSISTENT_DATA_ADDRESS 0x0800FC00  // Last page start (64KB - 1KB = 63KB = 0xFC00)
#define FIRST_BOOT_FLAG_OFFSET  0x00        // Offset within the page
#define FIRST_BOOT_MAGIC        0xDEADBEEF  // Magic value indicating first boot complete
#define FLASH_END_ADDRESS       0x08010000  // End of 64KB flash

// Structure for persistent data
typedef struct {
    uint32_t first_boot_magic;
    uint32_t padding[255];  // Fill rest of page to prevent accidental writes
} persistent_data_t;

static volatile persistent_data_t* persistent_data = (persistent_data_t*)PERSISTENT_DATA_ADDRESS;

// Check if this is the first boot
bool is_first_boot(void) {
    // Safety check: ensure we're reading from valid flash address
    if ((uint32_t)persistent_data < 0x08000000 || (uint32_t)persistent_data >= FLASH_END_ADDRESS) {
        DMESG("ERROR: Invalid persistent data address: 0x%08x", (uint32_t)persistent_data);
        return true; // Assume first boot on error
    }
    
    uint32_t magic = persistent_data->first_boot_magic;
    bool first_boot = (magic != FIRST_BOOT_MAGIC);
    
    DMESG("Persistent storage check: magic=0x%08x, first_boot=%d", magic, first_boot);
    return first_boot;
}

// Mark first boot as complete
void mark_first_boot_complete(void) {
    HAL_StatusTypeDef status;
    
    // Safety check: ensure we're writing to valid flash address
    if ((uint32_t)persistent_data < 0x08000000 || (uint32_t)persistent_data >= FLASH_END_ADDRESS) {
        DMESG("ERROR: Invalid persistent data address for write: 0x%08x", (uint32_t)persistent_data);
        return;
    }
    
    // Check if already marked (avoid unnecessary flash writes)
    if (persistent_data->first_boot_magic == FIRST_BOOT_MAGIC) {
        DMESG("First boot flag already set, skipping write");
        return;
    }
    
    DMESG("Writing first boot completion flag to flash at 0x%08x", PERSISTENT_DATA_ADDRESS);
    
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
        // Verify the write
        if (persistent_data->first_boot_magic == FIRST_BOOT_MAGIC) {
            DMESG("First boot flag verified successfully");
        } else {
            DMESG("WARNING: First boot flag verification failed");
        }
    }
    
    // Lock flash
    HAL_FLASH_Lock();
}
