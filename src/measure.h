#pragma once
#include "port_drv.h"
#include <stdbool.h>

typedef enum {
    COMP_NONE,
    COMP_OPEN,
    COMP_SHORT,
    COMP_RESISTOR,
    COMP_CAPACITOR,
    COMP_INDUCTOR,
    COMP_DIODE,
    COMP_ZENER,
    COMP_BJT_NPN,
    COMP_BJT_PNP,
    COMP_MOSFET_N,
    COMP_MOSFET_P,
    COMP_JFET_N,
    COMP_SCR,
    COMP_TRIAC,
    COMP_UNKNOWN
} component_type_t;

typedef struct {
    component_type_t type;
    port_id_t p1, p2, p3;     /* role mapping is per-type, see comments below */
    double value1;            /* Ohm / F / H / V depending on type */
    double value2;            /* hFE, Vth, etc. */
    char   note[24];
} component_result_t;

/* role conventions:
 *   RESISTOR/CAPACITOR/INDUCTOR : p1,p2 = the two leads
 *   DIODE/ZENER                 : p1 = anode, p2 = cathode
 *   BJT_xxx                     : p1 = base, p2 = collector, p3 = emitter
 *   MOSFET_xxx, JFET_N          : p1 = gate,  p2 = drain,     p3 = source
 *   SCR                         : p1 = gate,  p2 = anode,     p3 = cathode
 *   TRIAC                       : p1 = gate,  p2 = MT2,       p3 = MT1
 */

bool measure_resistor(port_id_t a, port_id_t b, double *r_ohm);
bool measure_capacitor(port_id_t a, port_id_t b, double *farad, double *esr_ohm);
bool find_diode(port_id_t a, port_id_t b); /* true if a(+)->b(-) conducts as diode */
double diode_vf_mv(port_id_t a, port_id_t b);

component_result_t identify_component(void);
