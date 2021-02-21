
#include <iwdg.h>

#include "tick.h"
#include "wdog.h"

#ifdef WDOG_MEASURE_INTERVAL
uint32_t max_wdog_interval = 0;
uint64_t last_tick = 0;
#endif

void wdog_init(void)
{
    iwdg_set_period_ms(200);

    iwdg_start();

#ifdef WDOG_MEASURE_INTERVAL
    last_tick = get_ticks();
#endif
}

void wdog_kick(void)
{
#ifdef WDOG_MEASURE_INTERVAL
    uint64_t tick = get_ticks();
    uint32_t interval = tick - last_tick;

    if (interval > max_wdog_interval)
    {
        max_wdog_interval = interval;
    }
    last_tick = tick;
#endif

    iwdg_reset();
}
