#include "measure.h"
#include "port_drv.h"
#include "config.h"
#include "pico/stdlib.h"
#include <math.h>
#include <string.h>

static const port_id_t ALL_P[3] = { PORT_TP1, PORT_TP2, PORT_TP3 };

static port_id_t third_pin(port_id_t a, port_id_t b)
{
    for (int i = 0; i < 3; i++)
        if (ALL_P[i] != a && ALL_P[i] != b) return ALL_P[i];
    return PORT_TP1; /* unreachable */
}

/* ---------------- diode primitives ---------------- */

double diode_vf_mv(port_id_t a, port_id_t b)
{
    port_rh(a, BIAS_VCC);
    port_set_low(b);
    sleep_us(600);
    double v = port_read_mv(a);
    port_rh(a, BIAS_HIZ);
    port_set_hiz(b);
    return v;
}

bool find_diode(port_id_t a, port_id_t b)
{
    double v = diode_vf_mv(a, b);
    return (v > 80.0 && v < 2600.0);
}

/* ---------------- BJT ---------------- */

static bool gain_npn(port_id_t base, port_id_t coll, port_id_t emit, double *hfe)
{
    port_set_low(emit);
    port_rl(base, BIAS_VCC);
    port_rh(coll, BIAS_VCC);
    sleep_ms(2);
    double vb = port_read_mv(base);
    double vc = port_read_mv(coll);
    port_rl(base, BIAS_HIZ);
    port_rh(coll, BIAS_HIZ);
    port_set_hiz(emit);

    double ib = ((double)VCC_MV - vb) / 1000.0 / R_HIGH_OHM;
    double ic = ((double)VCC_MV - vc) / 1000.0 / R_LOW_OHM;
    if (ib <= 1e-9) return false;
    *hfe = ic / ib;
    return (*hfe > 1.0 && *hfe < 2000.0);
}

static bool gain_pnp(port_id_t base, port_id_t coll, port_id_t emit, double *hfe)
{
    port_set_high(emit);
    port_rl(base, BIAS_GND);
    port_rh(coll, BIAS_GND);
    sleep_ms(2);
    double vb = port_read_mv(base);
    double vc = port_read_mv(coll);
    port_rl(base, BIAS_HIZ);
    port_rh(coll, BIAS_HIZ);
    port_set_hiz(emit);

    double ib = vb / 1000.0 / R_HIGH_OHM;
    double ic = vc / 1000.0 / R_LOW_OHM;
    if (ib <= 1e-9) return false;
    *hfe = ic / ib;
    return (*hfe > 1.0 && *hfe < 2000.0);
}

/* try both lead assignments for the two non-base pins, keep the higher gain */
static bool try_bjt(port_id_t base, port_id_t x, port_id_t y, bool npn,
                     port_id_t *coll, port_id_t *emit, double *hfe)
{
    double h1 = 0, h2 = 0;
    bool ok1 = npn ? gain_npn(base, x, y, &h1) : gain_pnp(base, x, y, &h1);
    bool ok2 = npn ? gain_npn(base, y, x, &h2) : gain_pnp(base, y, x, &h2);
    if (!ok1 && !ok2) return false;
    if (h1 >= h2) { *coll = x; *emit = y; *hfe = h1; }
    else          { *coll = y; *emit = x; *hfe = h2; }
    return true;
}

/* ---------------- MOSFET / JFET (enhancement, simplified) ---------------- */

static bool try_mosfet_n(port_id_t g, port_id_t d, port_id_t s)
{
    port_set_low(s);
    port_rh(d, BIAS_VCC);
    port_set_low(g);
    sleep_us(300);
    double v_off = port_read_mv(d);
    port_set_high(g);
    sleep_us(300);
    double v_on = port_read_mv(d);
    port_set_hiz(g); port_rh(d, BIAS_HIZ); port_set_hiz(s);
    return (v_off > (double)VCC_MV - 250.0) && (v_on < 800.0);
}

static bool try_mosfet_p(port_id_t g, port_id_t d, port_id_t s)
{
    port_set_high(s);
    port_rh(d, BIAS_GND);
    port_set_high(g);
    sleep_us(300);
    double v_off = port_read_mv(d);
    port_set_low(g);
    sleep_us(300);
    double v_on = port_read_mv(d);
    port_set_hiz(g); port_rh(d, BIAS_HIZ); port_set_hiz(s);
    return (v_off < 250.0) && (v_on > (double)VCC_MV - 800.0);
}

/* ---------------- SCR (gate-triggered latch, single quadrant) ---------------- */

static bool try_scr(port_id_t g, port_id_t a, port_id_t k, double *v_latched)
{
    port_set_low(k);
    port_rh(a, BIAS_VCC);
    sleep_us(400);
    double v_before = port_read_mv(a);
    if (v_before < (double)VCC_MV - 200.0) { port_rh(a, BIAS_HIZ); port_set_hiz(k); return false; }

    port_rh(g, BIAS_VCC);
    sleep_us(200);
    port_rh(g, BIAS_HIZ);
    sleep_us(500);
    double v_after = port_read_mv(a);
    port_rh(a, BIAS_HIZ);
    port_set_hiz(k);
    *v_latched = v_after;
    return v_after < 1500.0;
    /* TODO: triac = same test repeated with anode/cathode swapped and a
     * negative-gate variant (needs a negative-gate drive path to fully
     * confirm bidirectional triggering). */
}

/* ---------------- top-level identification ---------------- */

component_result_t identify_component(void)
{
    component_result_t r;
    memset(&r, 0, sizeof(r));
    r.type = COMP_UNKNOWN;
    all_ports_hiz();

    bool diode_ok[3][3] = {0};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (i != j) diode_ok[i][j] = find_diode(ALL_P[i], ALL_P[j]);

    int out_cnt[3] = {0}, in_cnt[3] = {0};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            if (i == j) continue;
            if (diode_ok[i][j]) out_cnt[i]++;
            if (diode_ok[j][i]) in_cnt[i]++;
        }

    /* --- NPN: base sources current into both other pins --- */
    for (int i = 0; i < 3; i++) {
        if (out_cnt[i] == 2) {
            port_id_t base = ALL_P[i];
            port_id_t x = ALL_P[(i + 1) % 3], y = ALL_P[(i + 2) % 3];
            port_id_t c, e; double hfe;
            if (try_bjt(base, x, y, true, &c, &e, &hfe)) {
                r.type = COMP_BJT_NPN; r.p1 = base; r.p2 = c; r.p3 = e;
                r.value1 = hfe; strcpy(r.note, "NPN");
                return r;
            }
        }
    }
    /* --- PNP: base sinks current from both other pins --- */
    for (int i = 0; i < 3; i++) {
        if (in_cnt[i] == 2) {
            port_id_t base = ALL_P[i];
            port_id_t x = ALL_P[(i + 1) % 3], y = ALL_P[(i + 2) % 3];
            port_id_t c, e; double hfe;
            if (try_bjt(base, x, y, false, &c, &e, &hfe)) {
                r.type = COMP_BJT_PNP; r.p1 = base; r.p2 = c; r.p3 = e;
                r.value1 = hfe; strcpy(r.note, "PNP");
                return r;
            }
        }
    }

    /* --- single junction => plain diode --- */
    int total_diode = 0, da = -1, db = -1;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (i != j && diode_ok[i][j]) { total_diode++; da = i; db = j; }
    if (total_diode == 1) {
        r.type = COMP_DIODE; r.p1 = ALL_P[da]; r.p2 = ALL_P[db];
        r.value1 = diode_vf_mv(ALL_P[da], ALL_P[db]);
        strcpy(r.note, "Vf,mV");
        return r;
    }

    /* --- no junctions: passive component or FET/SCR --- */
    if (total_diode == 0) {
        for (int i = 0; i < 3; i++) {
            port_id_t a = ALL_P[i], b = ALL_P[(i + 1) % 3];
            double rohm;
            if (measure_resistor(a, b, &rohm)) {
                r.type = COMP_RESISTOR; r.p1 = a; r.p2 = b; r.value1 = rohm;
                return r;
            }
        }
        for (int i = 0; i < 3; i++) {
            port_id_t a = ALL_P[i], b = ALL_P[(i + 1) % 3];
            double farad, esr;
            if (measure_capacitor(a, b, &farad, &esr)) {
                r.type = COMP_CAPACITOR; r.p1 = a; r.p2 = b;
                r.value1 = farad; r.value2 = esr;
                return r;
            }
        }
        /* FET sweep: try every pin as gate, both drain/source orderings */
        for (int i = 0; i < 3; i++) {
            port_id_t g = ALL_P[i], x = ALL_P[(i + 1) % 3], y = ALL_P[(i + 2) % 3];
            if (try_mosfet_n(g, x, y)) { r.type = COMP_MOSFET_N; r.p1 = g; r.p2 = x; r.p3 = y; return r; }
            if (try_mosfet_n(g, y, x)) { r.type = COMP_MOSFET_N; r.p1 = g; r.p2 = y; r.p3 = x; return r; }
            if (try_mosfet_p(g, x, y)) { r.type = COMP_MOSFET_P; r.p1 = g; r.p2 = x; r.p3 = y; return r; }
            if (try_mosfet_p(g, y, x)) { r.type = COMP_MOSFET_P; r.p1 = g; r.p2 = y; r.p3 = x; return r; }
        }
        /* SCR sweep: try every pin as gate, both anode/cathode orderings */
        for (int i = 0; i < 3; i++) {
            port_id_t g = ALL_P[i], x = ALL_P[(i + 1) % 3], y = ALL_P[(i + 2) % 3];
            double vlatch;
            if (try_scr(g, x, y, &vlatch)) { r.type = COMP_SCR; r.p1 = g; r.p2 = x; r.p3 = y; r.value1 = vlatch; return r; }
            if (try_scr(g, y, x, &vlatch)) { r.type = COMP_SCR; r.p1 = g; r.p2 = y; r.p3 = x; r.value1 = vlatch; return r; }
        }
        r.type = COMP_OPEN;
        return r;
    }

    /* multiple junctions but no clean BJT pattern (e.g. shorted leads, triac
     * candidate, or two unrelated diodes) */
    r.type = COMP_UNKNOWN;
    return r;
}
