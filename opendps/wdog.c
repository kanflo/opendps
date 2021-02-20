
#include <iwdg.h>

#include "tick.h"
#include "wdog.h"

void wdog_init(void)
{
    iwdg_set_period_ms(200);

    iwdg_start();
}

void wdog_kick(void)
{
    iwdg_reset();

}
