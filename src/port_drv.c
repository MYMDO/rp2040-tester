#include "port_drv.h"
#include "config.h"
#include "adc_drv.h"
#include "pico/stdlib.h"

static const uint8_t tp_pin[NUM_PORTS]  = { TP1_PIN,    TP2_PIN,    TP3_PIN };
static const uint8_t tp_adc[NUM_PORTS]  = { TP1_ADC_CH, TP2_ADC_CH, TP3_ADC_CH };
static const uint8_t rh_pin[NUM_PORTS]  = { TP1_RH_PIN, TP2_RH_PIN, TP3_RH_PIN };
static const uint8_t rl_pin[NUM_PORTS]  = { TP1_RL_PIN, TP2_RL_PIN, TP3_RL_PIN };

static void bias_pin(uint8_t pin, bias_t b)
{
    if (b == BIAS_HIZ) {
        gpio_set_dir(pin, GPIO_IN);
        gpio_disable_pulls(pin);
    } else {
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, b == BIAS_VCC);
    }
}

void port_init(void)
{
    adc_drv_init();
    for (int i = 0; i < NUM_PORTS; i++) {
        gpio_init(tp_pin[i]);
        gpio_init(rh_pin[i]);
        gpio_init(rl_pin[i]);
        gpio_set_dir(tp_pin[i], GPIO_IN);
        gpio_disable_pulls(tp_pin[i]);
        bias_pin(rh_pin[i], BIAS_HIZ);
        bias_pin(rl_pin[i], BIAS_HIZ);
    }
}

void port_set_hiz(port_id_t p)
{
    gpio_set_dir(tp_pin[p], GPIO_IN);
    gpio_disable_pulls(tp_pin[p]);
}

void port_set_low(port_id_t p)
{
    gpio_set_dir(tp_pin[p], GPIO_OUT);
    gpio_put(tp_pin[p], 0);
}

void port_set_high(port_id_t p)
{
    gpio_set_dir(tp_pin[p], GPIO_OUT);
    gpio_put(tp_pin[p], 1);
}

void port_rh(port_id_t p, bias_t b) { bias_pin(rh_pin[p], b); }
void port_rl(port_id_t p, bias_t b) { bias_pin(rl_pin[p], b); }

uint16_t port_read_raw(port_id_t p)
{
    return adc_drv_read(tp_adc[p]);
}

uint16_t port_read_mv(port_id_t p)
{
    return adc_drv_raw_to_mv(port_read_raw(p));
}

void all_ports_hiz(void)
{
    for (int i = 0; i < NUM_PORTS; i++) {
        port_set_hiz((port_id_t)i);
        port_rh((port_id_t)i, BIAS_HIZ);
        port_rl((port_id_t)i, BIAS_HIZ);
    }
}
