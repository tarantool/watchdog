#define _GNU_SOURCE 1

#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <dlfcn.h>

#include <lua.h>
#include <lauxlib.h>
#include <module.h> // tarantool

volatile static bool enable_coredump = false;
volatile static double pettime = 0.;
volatile static double timeout = 0.;
static struct fiber *f_petting = NULL;
static struct fiber *f_timer = NULL;

static bool is_box_on_shutdown_available = false;
static bool is_on_shutdown_trigger_active = false;
static int (*box_on_shutdown_ptr)(void *, int (*)(void *), int (*)(void *));

static ssize_t
coio_timer(va_list ap)
{
	(void)ap;

	double prev = clock_monotonic();
	double tt;
	while (tt = timeout, tt) {
		double now = clock_monotonic();

		if (now > pettime + tt) {
			if (now - prev > 1) {
				// nanosleep took > 1 sec instead of 200ms
				// maybe system was suspended for a while
				// thus timeout should be ignored once
				pettime = now;
			} else {
				say_error("Watchdog timeout %.1f sec. Aborting", tt);
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
		fiber_sleep(timeout/2.);
	}

	return 0;
}

static void
stop_impl()
{
	timeout = 0;

	if (f_petting != NULL) {
		fiber_cancel(f_petting);
		f_petting = NULL;
	}

	if (f_timer != NULL) {
		fiber_join(f_timer);
		f_timer = NULL;
	}
}

static int
on_stop_trigger(void *arg)
{
	(void)arg;
	stop_impl();
	return 0;
}

static int
stop(lua_State *L)
{
	(void)L;
	stop_impl();

	if (is_box_on_shutdown_available && is_on_shutdown_trigger_active) {
		box_on_shutdown_ptr(NULL, NULL, on_stop_trigger);
		is_on_shutdown_trigger_active = false;
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

	if (is_box_on_shutdown_available && !is_on_shutdown_trigger_active) {
		box_on_shutdown_ptr(NULL, on_stop_trigger, NULL);
		is_on_shutdown_trigger_active = true;
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
	/**
	 * For unknown reason Tarantool version before 2.8 has also "box_on_shutdown"
	 * symbol (at least on MacOS). Checked with `nm tarantool|grep shutdown`.
	 * Therefore initially we check "box_on_shutdown_trigger_list" here
	 * such symbol exists only in 2.8+ version where "box_on_shutdown" trigger
	 * is implemented.
	 */
	if (dlsym(RTLD_DEFAULT, "box_on_shutdown_trigger_list") != NULL) {
		box_on_shutdown_ptr = dlsym(RTLD_DEFAULT, "box_on_shutdown");
		if (box_on_shutdown_ptr != NULL) {
			is_box_on_shutdown_available = true;
		}
	}

	static const struct luaL_Reg lib [] = {
		{"start", start},
		{"stop", stop},
		{NULL, NULL}
	};
	luaL_newlib(L, lib);
	atexit(watchdog_atexit);
	return 1;
}
