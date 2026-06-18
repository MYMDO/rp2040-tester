#pragma once
#include <stdint.h>

void     adc_drv_init(void);
uint16_t adc_drv_read(uint8_t channel);       /* 12-bit, 8x oversampled */
uint16_t adc_drv_raw_to_mv(uint16_t raw);
