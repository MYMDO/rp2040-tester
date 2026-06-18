#include "adc_drv.h"
#include "config.h"
#include "hardware/adc.h"
#include "pico/stdlib.h"

void adc_drv_init(void)
{
    adc_init();
    adc_set_temp_sensor_enabled(false);
    adc_set_clkdiv(0);
}

uint16_t adc_drv_read(uint8_t channel)
{
    adc_select_input(channel);
    uint32_t sum = 0;
    const int n = 8;
    for (int i = 0; i < n; i++) {
        sum += adc_read();
        sleep_us(2);
    }
    return (uint16_t)(sum / n);
}

/* NOTE: RP2040 ADC reference is the board's 3V3 rail (no internal precision
 * bandgap like AVR's 1.1V). Replace VCC_MV with a value measured against a
 * calibrated meter for better absolute accuracy, or add a resistor-divider
 * reference channel and calibrate at runtime. */
uint16_t adc_drv_raw_to_mv(uint16_t raw)
{
    return (uint16_t)(((uint32_t)raw * VCC_MV) / ADC_MAX);
}
