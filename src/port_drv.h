#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum { PORT_TP1 = 0, PORT_TP2 = 1, PORT_TP3 = 2 } port_id_t;
typedef enum { BIAS_HIZ, BIAS_GND, BIAS_VCC } bias_t;

void     port_init(void);
void     port_set_hiz(port_id_t p);
void     port_set_low(port_id_t p);   /* direct digital drive to 0V  */
void     port_set_high(port_id_t p);  /* direct digital drive to 3.3V */
void     port_rh(port_id_t p, bias_t b); /* 680R   switch to GND/VCC/HiZ */
void     port_rl(port_id_t p, bias_t b); /* 470k   switch to GND/VCC/HiZ */
uint16_t port_read_raw(port_id_t p);     /* 12-bit ADC, 8x oversampled  */
uint16_t port_read_mv(port_id_t p);
void     all_ports_hiz(void);
