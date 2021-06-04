#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include <lua.h>
#include <lauxlib.h>
#include <module.h> // tarantool

volatile static bool enable_coredump = false;
volatile static double pettime = 0.;
volatile static double timeout = 0.;
static struct fiber *f_petting = NULL;
static struct fiber *f_timer = NULL;

static ssize_t
coio_timer(va_list ap)
{
	(void)ap;

	double prev = clock_monotonic();
	double tt;
	while (tt = timeout, tt) {
		double now = clock_monotonic();

		// Pettime is updated every timeout/4 seconds. We want the
		// real timeout event to occur not earlier than timeout,
		// thus add spare 25% here.
		if (now > pettime + (1.25 * tt)) {
			if (now - prev > 1) {
				// nanosleep took > 1 sec instead of 200ms
				// maybe system was suspended for a while
				// thus timeout should be ignored once
				pettime = now;
			} else {
				say_error("Watchdog timeout %.1f sec. Aborting", now - pettime);
				/** after exit() process doesn't save coredump but abort does */
				if (enable_coredump)
					abort();
				else
					exit(SIGABRT);
			}
		} else {
			struct timespec ms200 = {.tv_nsec = 200L * 1000 * 1000};
			nanosleep(&ms200, NULL);
		}

		prev = now;
	}

	say_info("Watchdog stopped");
	return 0;
}

static int
fiber_timer(va_list ap)
{
	(void)ap;
	fiber_set_joinable(fiber_self(), true);
	return coio_call(coio_timer);
}

static int
fiber_petting(va_list ap)
{
	(void)ap;
	fiber_set_cancellable(true);
	while (!fiber_is_cancelled() && timeout) {
		pettime = clock_monotonic();
		fiber_sleep(timeout/4.);
	}

	return 0;
}


static int
start(lua_State *L)
{
	double t = luaL_checknumber(L, 1);
	if (!isfinite(t) || t < 0) {
		return luaL_argerror(L, 1, "timeout must be positive");
	}

	if (lua_gettop(L) == 2) {
		enable_coredump = lua_toboolean(L, 2);
	}

	if (timeout != 0) {
		timeout = t;
		pettime = clock_monotonic();

		say_info("Watchdog timeout changed to %.1f sec (coredump %s)",
		   timeout, enable_coredump ? "enabled" : "disabled");
	} else {
		timeout = t;

		assert(f_petting == NULL);
		f_petting = fiber_new("watchdog_petting", fiber_petting);
		fiber_start(f_petting);

		assert(f_timer == NULL);
		f_timer = fiber_new("watchdog_timer", fiber_timer);
		fiber_start(f_timer);

		say_info("Watchdog started with timeout %.1f sec (coredump %s)",
		   timeout, enable_coredump ? "enabled" : "disabled");
	}

	return 0;
}

static int
stop(lua_State *L)
{
	(void)L;
	timeout = 0;

	if (f_petting != NULL) {
		fiber_cancel(f_petting);
		f_petting = NULL;
	}

	if (f_timer != NULL) {
		fiber_join(f_timer);
		f_timer = NULL;
	}

	return 0;
}

static void
watchdog_atexit(void)
{
	timeout = 0;
}

/* ====================LIBRARY INITIALISATION FUNCTION======================= */

LUA_API int
luaopen_watchdog(lua_State *L)
{

	lua_getglobal(L, "box"); // -0 +1
	if (!lua_istable(L, -1)) {
		return luaL_error(L, "assertion failed! missing global 'box'");
	}

	lua_pushstring(L, "ctl"); // -0 +1
	lua_rawget(L, -2); // -1 +1
	if (!lua_istable(L, -1)) {
		return luaL_error(L, "assertion failed! missing module 'box.ctl'");
	}

	lua_getfield(L, -1, "on_shutdown"); // -0 +1

	if (lua_isfunction(L, -1)) {
		lua_pushcfunction(L, stop); // -0 +1
		lua_call(L, 1, 0); // -2 +0
	} else {
		atexit(watchdog_atexit);
	}

	static const struct luaL_Reg lib [] = {
		{"start", start},
		{"stop", stop},
		{NULL, NULL}
	};

	luaL_newlib(L, lib); // -0 +1

	return 1;
}
