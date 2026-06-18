#include "measure.h"
#include "port_drv.h"
#include "config.h"
#include "pico/stdlib.h"
#include <math.h>

#define CHARGE_TIMEOUT_US   1500000ULL

/* discharge DUT fully through both ports before any test */
static void discharge(port_id_t a, port_id_t b)
{
    port_set_low(a);
    port_set_low(b);
    sleep_ms(3);
    port_set_hiz(a);
    port_set_hiz(b);
}

/* charge through resistor R on port a (Vcc side), port b held at GND.
 * returns elapsed microseconds to reach 50% Vcc, or 0 if never reached. */
static uint64_t charge_to_half(port_id_t a, port_id_t b, bool use_rh)
{
    discharge(a, b);
    port_set_low(b);
    if (use_rh) port_rh(a, BIAS_VCC); else port_rl(a, BIAS_VCC);

    uint64_t t0 = time_us_64();
    uint64_t elapsed = 0;
    while (1) {
        uint16_t mv = port_read_mv(a);
        elapsed = time_us_64() - t0;
        if (mv >= VCC_MV / 2) break;
        if (elapsed > CHARGE_TIMEOUT_US) { elapsed = 0; break; }
    }
    if (use_rh) port_rh(a, BIAS_HIZ); else port_rl(a, BIAS_HIZ);
    port_set_hiz(b);
    return elapsed;
}

bool measure_capacitor(port_id_t a, port_id_t b, double *farad, double *esr_ohm)
{
    uint64_t t_us = charge_to_half(a, b, true);   /* RH = 680R, good for nF..mF */
    double r_used = R_LOW_OHM;

    if (t_us != 0 && t_us < 30) {
        /* too fast for 680R, too small to be useful, try RL for tiny caps */
        t_us = charge_to_half(a, b, false);
        r_used = R_HIGH_OHM;
        if (t_us == 0) return false; /* not a measurable capacitor */
    } else if (t_us == 0) {
        return false; /* didn't charge at all within timeout -> not C, or huge leak */
    }

    double tau = (double)t_us * 1e-6 / M_LN2;  /* t = R*C*ln(2) */
    *farad = tau / r_used;

    /* --- simplified ESR estimate (electrolytics only, R=680 path) --- */
    *esr_ohm = -1.0; /* unknown by default */
    if (*farad > 1e-6) {
        discharge(a, b);
        port_set_low(b);
        port_rh(a, BIAS_VCC);
        sleep_us(4);                 /* sample before bulk C charges meaningfully */
        double v_jump = port_read_mv(a);
        port_rh(a, BIAS_HIZ);
        port_set_hiz(b);
        if (v_jump > 1 && v_jump < VCC_MV - 1)
            *esr_ohm = R_LOW_OHM * (v_jump / (VCC_MV - v_jump));
        /* NOTE: needs calibration against a known-ESR reference cap */
    }
    return true;
}
