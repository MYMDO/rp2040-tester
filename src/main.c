#include "pico/stdlib.h"
#include "port_drv.h"
#include "ssd1306.h"
#include "ui.h"

int main(void)
{
    stdio_init_all();
    port_init();
    ssd1306_init();
    ui_init();

    while (1) {
        ui_loop();
    }
    return 0;
}
