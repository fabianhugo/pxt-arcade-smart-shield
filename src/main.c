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
    screen_set_backlight(255);
    
    // Show test screen for 5 seconds
    uint64_t start_time = tim_get_micros();
    uint64_t duration = 5000000; // 5 seconds in microseconds
    
    while (tim_get_micros() - start_time < duration) {
        screen_stripes();
    }
    
    DMESG("Test screen displayed for first boot");
}

int main(void) {
    jdspi_early_init();
    led_init();

    tim_init();
    // dspi_init();
    adc_init_random();

    tick();

    jdspi_init();

    // Check if this is the first boot and show test screen
    if (is_first_boot()) {
        DMESG("First boot detected - showing test screen");
        show_test_screen();
        mark_first_boot_complete();
        DMESG("First boot complete flag set");
    } else {
        DMESG("Not first boot - skipping test screen");
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
