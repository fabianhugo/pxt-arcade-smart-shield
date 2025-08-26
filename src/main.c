// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jdsimple.h"
#include "jdspi.h"
#include "persistent.h"



void led_init() {
    pin_setup_output(PIN_LOG0);
    pin_setup_output(PIN_LOG1);
    pin_setup_output(PIN_LED);
}

void set_log_pin(int v) {
    pin_set(PIN_LOG0, v);
}

void set_log_pin2(int v) {
    pin_set(PIN_LOG1, v);
}

void pulse_log_pin() {
    set_log_pin(1);
    set_log_pin(0);
}

void pulse_log_pin2() {
    set_log_pin2(1);
    set_log_pin2(0);
}

void led_toggle() {
    pin_toggle(PIN_LED);
}

void led_set(int state) {
    pin_set(PIN_LED, state);
}

// uint8_t crcBuf[1024];

static void tick() {
    // pulse_log_pin();
    tim_set_timer(10000, tick);
}

void screen_stripes(void);

void show_test_screen(void) {
    DMESG("Starting first boot test screen");
    screen_set_backlight(255);
    
    // Show test screen for 2 seconds
    uint64_t start_time = tim_get_micros();
    uint64_t duration = 2000000; // 2 seconds in microseconds
    uint64_t last_update = start_time;
    
    while (tim_get_micros() - start_time < duration) {
        uint64_t current_time = tim_get_micros();
        
        // Update screen every 100ms to ensure it's visible
        if (current_time - last_update > 100000) {
            screen_stripes();
            last_update = current_time;
        }
        
        // Allow other processing during the display
        // Small delay to prevent busy waiting
        wait_us(1000);
    }
    
    DMESG("Test screen completed after 5 seconds");
}

int main(void) {
    // Set up backlight pin FIRST to prevent any flicker during boot
#ifdef PROTO_V2
    pin_setup_output(PB_0);  // PIN_DISPLAY_BL for PROTO_V2
    pin_set(PB_0, 0);
#else
    pin_setup_output(PA_10); // PIN_DISPLAY_BL for non-PROTO_V2
    pin_set(PA_10, 0);
#endif
    
    jdspi_early_init();
    led_init();

    tim_init();
    // dspi_init();
    adc_init_random();

    tick();

    jdspi_init();

    // Check if this is the first boot and show test screen
    bool first_boot = is_first_boot();
    DMESG("First boot check result: %s", first_boot ? "TRUE (showing test screen)" : "FALSE (normal boot)");
    
    if (first_boot) {
        show_test_screen();
        mark_first_boot_complete();
        
        // Verify the flag was set correctly
        if (!is_first_boot()) {
            DMESG("First boot process completed successfully");
        } else {
            DMESG("WARNING: First boot flag may not have been set correctly");
        }
    } else {
        DMESG("Normal boot - skipping test screen");
        // Set a default backlight level for normal operation
        screen_set_backlight(128);
    }
    // show_test_screen();

    uint64_t lastBlink = tim_get_micros();
    while (1) {
        if (tim_get_micros() - lastBlink > 300000) {
            lastBlink = tim_get_micros();
            led_toggle();
        }

        jdspi_process();
    }
}

void panic(void) {
    DMESG("PANIC!");
    target_disable_irq();
    while (1) {
        led_toggle();
        wait_us(100000);
    }
}

void jd_panic() {
    panic();
}
