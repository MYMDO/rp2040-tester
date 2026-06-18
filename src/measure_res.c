#include "measure.h"
#include "port_drv.h"
#include "config.h"
#include "pico/stdlib.h"
#include <math.h>

/* Vcc --[Rref]-- node(ADC) --DUT-- GND   =>  Rdut = Rref * Vnode/(Vcc-Vnode) */
static double divider_to_r(double vcc_mv, double vnode_mv, double rref_ohm)
{
    if (vnode_mv < 2.0)            vnode_mv = 2.0;
    if (vnode_mv > vcc_mv - 2.0)   vnode_mv = vcc_mv - 2.0;
    return rref_ohm * (vnode_mv / (vcc_mv - vnode_mv));
}

/* measure with a given reference resistor (RH or RL), a biased to VCC, b grounded */
static double measure_one_dir(port_id_t a, port_id_t b, bool use_rh)
{
    if (use_rh) port_rh(a, BIAS_VCC); else port_rl(a, BIAS_VCC);
    port_set_low(b);
    sleep_us(800);
    double vmv = port_read_mv(a);
    if (use_rh) port_rh(a, BIAS_HIZ); else port_rl(a, BIAS_HIZ);
    port_set_hiz(b);
    return divider_to_r(VCC_MV, vmv, use_rh ? R_LOW_OHM : R_HIGH_OHM);
}

bool measure_resistor(port_id_t a, port_id_t b, double *r_ohm)
{
    /* first pass with RL (470k) - good for >1k, then refine with RH if low */
    double r_fwd = measure_one_dir(a, b, false);
    bool low_range = (r_fwd < 5000.0);
    if (low_range) r_fwd = measure_one_dir(a, b, true);

    double r_rev = measure_one_dir(b, a, low_range);

    if (r_fwd < 0.5 || r_fwd > 22e6) return false;

    /* a true resistor is symmetric; a diode/junction is not */
    double diff = fabs(r_fwd - r_rev) / fmax(r_fwd, r_rev);
    if (diff > 0.20) return false;

    *r_ohm = (r_fwd + r_rev) / 2.0;
    return true;
}
