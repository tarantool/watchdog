#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include <lua.h>
#include <lauxlib.h>
#include <module.h> // tarantool

volatile static double pettime = 0.;
volatile static double timeout = 0.;
static struct fiber* f_petting;
static struct fiber* f_timer;

static ssize_t coio_timer(va_list ap)
{
	double tt;
	while (tt = timeout) {
		double now = clock_monotonic();

		if (now > pettime + tt) {
			say_error("Watchdog timeout %.1f sec. Aborting", tt);
			exit(6); // because SIGABRT == 6
		} else {
			struct timespec ms200 = {.tv_nsec = 200L*1000*1000};
			nanosleep(&ms200, NULL);
		}
	}

	say_info("Watchdog stopped");
	return 0;
}

static int fiber_timer(va_list ap)
{
	fiber_set_joinable(fiber_self(), true);
	return coio_call(coio_timer);
}

static int fiber_petting(va_list ap)
{
	fiber_set_cancellable(true);
	while (!fiber_is_cancelled() && timeout) {
		pettime = clock_monotonic();
		fiber_sleep(timeout/2.);
	}

	return 0;
}


int start(lua_State *L)
{
	double t = luaL_checknumber(L, 1);
	if (!isfinite(t) || t < 0) {
		return luaL_argerror(L, 1, "timeout must be positive");
	}
	
	if (timeout) {
		timeout = t;
		pettime = clock_monotonic();
		
		say_info("Watchdog timeout changed to %.1f sec", timeout);
	} else {
		timeout = t;

		assert(f_petting == NULL);
		f_petting = fiber_new("watchdog_petting", fiber_petting);
		fiber_start(f_petting);

		assert(f_timer == NULL);
		f_timer = fiber_new("watchdog_timer", fiber_timer);
		fiber_start(f_timer);

		say_info("Watchdog started with timeout %.1f sec", timeout);        
	}

	return 0;
}

int stop(lua_State *L)
{
	timeout = 0;

	fiber_cancel(f_petting);
	f_petting = NULL;

	fiber_join(f_timer);
	f_timer = NULL;

	return 0;
}

void watchdog_atexit(void)
{
	timeout = 0;
	return;
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
