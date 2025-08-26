// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef PERSISTENT_H
#define PERSISTENT_H

#include <stdbool.h>

// Check if this is the first boot since firmware installation
bool is_first_boot(void);

// Mark first boot as complete (call after showing test screen)
void mark_first_boot_complete(void);

#endif // PERSISTENT_H
