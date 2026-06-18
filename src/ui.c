#include "ui.h"
#include "config.h"
#include "port_drv.h"
#include "measure.h"
#include "ssd1306.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#define IDLE_OFF_MS   60000   /* auto power-off after 60s of inactivity */

static uint64_t last_activity_ms;

static void fmt_ohm(double r, char *buf, size_t n)
{
    if      (r >= 1e6) snprintf(buf, n, "%.2fMOHM", r / 1e6);
    else if (r >= 1e3) snprintf(buf, n, "%.2fKOHM", r / 1e3);
    else                snprintf(buf, n, "%.1fOHM",  r);
}

static void fmt_farad(double f, char *buf, size_t n)
{
    if      (f < 1e-9) snprintf(buf, n, "%.1fPF", f * 1e12);
    else if (f < 1e-6) snprintf(buf, n, "%.1fNF", f * 1e9);
    else                snprintf(buf, n, "%.1fUF", f * 1e6);
}

static void beep(int ms)
{
    gpio_put(BUZZER_PIN, 1);
    sleep_ms(ms);
    gpio_put(BUZZER_PIN, 0);
}

void ui_init(void)
{
    gpio_init(BTN_START_PIN);
    gpio_set_dir(BTN_START_PIN, GPIO_IN);
    gpio_pull_up(BTN_START_PIN);

    gpio_init(BTN_MODE_PIN);
    gpio_set_dir(BTN_MODE_PIN, GPIO_IN);
    gpio_pull_up(BTN_MODE_PIN);

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);

    gpio_init(PWR_LATCH_PIN);
    gpio_set_dir(PWR_LATCH_PIN, GPIO_OUT);
    gpio_put(PWR_LATCH_PIN, 1); /* keep board powered, if wired to a latch FET */

    ssd1306_clear();
    ssd1306_draw_string(0, 0, "RP2040 TESTER");
    ssd1306_draw_string(0, 16, "PRESS START");
    ssd1306_display();

    last_activity_ms = to_ms_since_boot(get_absolute_time());
}

static void run_test_and_show(void)
{
    ssd1306_clear();
    ssd1306_draw_string(0, 0, "TESTING");
    ssd1306_display();

    component_result_t r = identify_component();
    all_ports_hiz();

    char line1[24] = "UNKNOWN";
    char line2[24] = "";

    switch (r.type) {
    case COMP_RESISTOR:  strcpy(line1, "RESISTOR");  fmt_ohm(r.value1, line2, sizeof line2); break;
    case COMP_CAPACITOR: strcpy(line1, "CAPACITOR"); fmt_farad(r.value1, line2, sizeof line2); break;
    case COMP_DIODE:     strcpy(line1, "DIODE");     snprintf(line2, sizeof line2, "VF %dMV", (int)r.value1); break;
    case COMP_BJT_NPN:   strcpy(line1, "NPN");        snprintf(line2, sizeof line2, "HFE %d", (int)r.value1); break;
    case COMP_BJT_PNP:   strcpy(line1, "PNP");        snprintf(line2, sizeof line2, "HFE %d", (int)r.value1); break;
    case COMP_MOSFET_N:  strcpy(line1, "MOSFET-N");   break;
    case COMP_MOSFET_P:  strcpy(line1, "MOSFET-P");   break;
    case COMP_SCR:       strcpy(line1, "SCR");        break;
    case COMP_OPEN:      strcpy(line1, "OPEN");       break;
    default:             strcpy(line1, "UNKNOWN");    break;
    }

    ssd1306_clear();
    ssd1306_draw_string(0, 0, line1);
    ssd1306_draw_string(0, 16, line2);
    ssd1306_display();

    printf("RESULT: %s %s\n", line1, line2); /* USB-CDC log, full precision if needed */
    beep(40);
}

void ui_loop(void)
{
    static bool prev_pressed = false;
    bool pressed = !gpio_get(BTN_START_PIN); /* active low */

    if (pressed && !prev_pressed) {
        last_activity_ms = to_ms_since_boot(get_absolute_time());
        run_test_and_show();
        sleep_ms(150); /* simple debounce */
    }
    prev_pressed = pressed;

    uint64_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_activity_ms > IDLE_OFF_MS) {
        ssd1306_clear();
        ssd1306_draw_string(0, 0, "OFF");
        ssd1306_display();
        gpio_put(PWR_LATCH_PIN, 0); /* cuts board power if latch FET is wired */
        while (1) sleep_ms(1000);   /* halt; a fresh button wake-up needs external latch wiring, TODO */
    }
    sleep_ms(10);
}
