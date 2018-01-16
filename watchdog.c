#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include <lua.h>
#include <lauxlib.h>
#include <module.h> // tarantool

volatile static double pettime = 0.;
static double timeout = 0.;

static ssize_t coio_timer(va_list ap)
{
    say_info("Watchdog started with timeout %.1f sec", timeout);

    while (timeout) {
        double now = clock_monotonic();

        if (now > pettime + timeout) {
            say_error("Watchdog timeout %.1f sec. Aborting", timeout);
            abort();
        } else {
            sleep(pettime + timeout - now + 1);
        }
    }

    say_info("Watchdog stopped");
    return 0;

    #undef NANOSECONDS
    #undef SECONDS
}

static int fiber_timer(va_list ap)
{
    fiber_set_cancellable(true);
    int rv = coio_call(coio_timer, timeout);
    say_info("coio_call() returned %d", rv);
    return 0;
}

static int fiber_petting(va_list ap)
{
    fiber_set_cancellable(true);
    while (!fiber_is_cancelled()) {
        pettime = clock_monotonic();
        fiber_sleep(timeout/2.);
    }
    say_info("fiber_petting() returning");

    return 0;
}

static struct fiber* f_petting;// = fiber_new("watchdog_petting", fiber_petting);
static struct fiber* f_timer; // = fiber_new("watchdog_timer", fiber_timer);

int start(lua_State *L)
{
    double t = luaL_checknumber(L, 1);
    if (!isfinite(t) || t < 0) {
        return luaL_argerror(L, 1, "timeout must be positive");
    }
    
    if (timeout) {
        pettime = clock_monotonic();
        timeout = t;
        say_info("Watchdog timeout changed to %.1f sec", timeout);
        return 0;
    }

    timeout = t;
    f_petting = fiber_new("watchdog_petting", fiber_petting);
    fiber_start(f_petting);

    f_timer = fiber_new("watchdog_timer", fiber_timer);
    fiber_start(f_timer);

    return 0;
}

int stop(lua_State *L)
{
    timeout = 0;
    fiber_cancel(f_petting);
    fiber_cancel(f_timer);
    return 0;
}

void watchdog_atexit(void)
{
    timeout = 0;
    say_error("WATCHDOG ATAXIT");
}

/* ====================LIBRARY INITIALISATION FUNCTION======================= */

int luaopen_watchdog(lua_State *L)
{
    static const struct luaL_Reg lib [] = {
        {"start", start},
        {"stop", stop},
        {NULL, NULL}
    };
    luaL_newlib(L, lib);
    atexit(watchdog_atexit);
    return 1;
}
