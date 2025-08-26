// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef PERSISTENT_H
#define PERSISTENT_H

#include <stdbool.h>

// Check if this is the first boot since firmware installation
bool is_first_boot(void);

// Mark first boot as complete (call after showing test screen)
void mark_first_boot_complete(void);

/*
 * Alternative simpler approaches for first boot detection:
 * 
 * 1. CRC-based approach: Calculate CRC of specific RAM content on startup
 * 2. GPIO pin approach: Check specific GPIO pin state on boot
 * 3. Simple pattern approach: Write specific pattern to RAM and check
 * 
 * Current flash-based approach is most robust for production use.
 */

#endif // PERSISTENT_H
