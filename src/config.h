#pragma once
#include <stdint.h>

/* ---- Test ports (ADC-capable GPIOs) ---- */
#define TP1_PIN     26
#define TP2_PIN     27
#define TP3_PIN     28
#define TP1_ADC_CH  0
#define TP2_ADC_CH  1
#define TP3_ADC_CH  2
#define NUM_PORTS   3

/* ---- Bias resistor switch pins (digital only, each drives TPx through a fixed R) ---- */
#define TP1_RH_PIN  2   /* 680R   */
#define TP1_RL_PIN  3   /* 470k   */
#define TP2_RH_PIN  4
#define TP2_RL_PIN  5
#define TP3_RH_PIN  6
#define TP3_RL_PIN  7

/* ---- OLED (I2C0) ---- */
#define I2C_PORT_NUM   0
#define I2C_SDA_PIN    8
#define I2C_SCL_PIN    9
#define OLED_ADDR      0x3C
#define OLED_W         128
#define OLED_H         64

/* ---- UI ---- */
#define BTN_START_PIN  10
#define BTN_MODE_PIN   11
#define BUZZER_PIN     12
#define PWR_LATCH_PIN  13   /* optional self power-off, set high to stay on */

/* ---- Electrical constants ---- */
#define VCC_MV        3300u
#define ADC_MAX       4095u
#define R_LOW_OHM     680u      /* RH */
#define R_HIGH_OHM    470000u   /* RL */
